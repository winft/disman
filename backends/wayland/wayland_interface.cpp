/*************************************************************************
Copyright © 2020   Roman Gilg <subdiff@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
**************************************************************************/
#include "wayland_interface.h"

#include "waylandbackend.h"
#include "waylandoutput.h"
#include "waylandscreen.h"

#include <configmonitor.h>
#include <mode.h>

#include "wayland_logging.h"

#include <QThread>
#include <QTimer>
#include <Wrapland/Client/connection_thread.h>
#include <Wrapland/Client/event_queue.h>
#include <Wrapland/Client/registry.h>
#include <Wrapland/Client/wlr_output_configuration_v1.h>
#include <Wrapland/Client/wlr_output_manager_v1.h>

using namespace Disman;

WaylandInterface::WaylandInterface(QThread* thread)
    : m_dismanConfig{new Config}
{
    m_connection = new Wrapland::Client::ConnectionThread;

    connect(
        m_connection,
        &Wrapland::Client::ConnectionThread::establishedChanged,
        this,
        [this](bool established) {
            if (established) {
                setupRegistry();
            } else {
                handleDisconnect();
            }
        },
        Qt::QueuedConnection);

    connect(m_connection, &Wrapland::Client::ConnectionThread::failed, this, [this] {
        qCWarning(DISMAN_WAYLAND) << "Failed to connect to Wayland server at socket:"
                                  << m_connection->socketName();
        Q_EMIT connectionFailed(m_connection->socketName());
    });

    thread->start();
    m_connection->moveToThread(thread);
    m_connection->establishConnection();
}

void WaylandInterface::handleDisconnect()
{
    for (auto& [key, out] : m_outputMap) {
        delete out;
    }
    m_outputMap.clear();

    // Clean up
    if (m_queue) {
        delete m_queue;
        m_queue = nullptr;
    }

    m_connection->deleteLater();
    m_connection = nullptr;

    qCWarning(DISMAN_WAYLAND) << "Wayland disconnected, cleaning up.";
    Q_EMIT config_changed();
}

void WaylandInterface::setupRegistry()
{
    m_queue = new Wrapland::Client::EventQueue(this);
    m_queue->setup(m_connection);

    m_registry = new Wrapland::Client::Registry(this);

    connect(m_registry,
            &Wrapland::Client::Registry::wlrOutputManagerV1Announced,
            this,
            [this](quint32 name, quint32 version) {
                m_outputManager = m_registry->createWlrOutputManagerV1(name, version, m_registry);

                connect(m_outputManager,
                        &Wrapland::Client::WlrOutputManagerV1::head,
                        this,
                        &WaylandInterface::add_output);

                connect(m_outputManager,
                        &Wrapland::Client::WlrOutputManagerV1::done,
                        this,
                        &WaylandInterface::handle_wlr_manager_done);
                m_outputManager->setEventQueue(m_queue);
            });

    m_registry->setEventQueue(m_queue);
    m_registry->create(m_connection);
    m_registry->setup();
}

void WaylandInterface::add_output(Wrapland::Client::WlrOutputHeadV1* head)
{
    auto output = new WaylandOutput(++m_outputId, *head);
    m_outputMap.insert({output->id, output});
    update.outputs = true;

    connect(output, &WaylandOutput::removed, this, [this, output] { removeOutput(output); });
}

void WaylandInterface::removeOutput(WaylandOutput* output)
{
    update.outputs = true;
    m_outputMap.erase(output->id);
    delete output;
}

void WaylandInterface::handle_wlr_manager_done()
{
    is_initialized = true;
    update.pending = false;

    if (update.outputs) {
        update.outputs = false;
        Q_EMIT outputsChanged();
    }

    Q_EMIT config_changed();
    tryPendingConfig();
}

void WaylandInterface::updateConfig(Disman::ConfigPtr& config)
{
    config->set_supported_features(Config::Feature::Writable | Config::Feature::PerOutputScaling);
    config->set_valid(m_connection->display());

    // Removing removed outputs
    for (auto const& [key, output] : config->outputs()) {
        if (m_outputMap.find(output->id()) == m_outputMap.end()) {
            config->remove_output(output->id());
        }
    }

    // Add Disman::Outputs that aren't in the list yet.
    auto dismanOutputs = config->outputs();

    for (auto& [key, output] : m_outputMap) {
        Disman::OutputPtr dismanOutput;

        auto it = dismanOutputs.find(output->id);
        if (it == dismanOutputs.end()) {
            dismanOutput = output->toDismanOutput();
            dismanOutputs.insert({dismanOutput->id(), dismanOutput});
        } else {
            dismanOutput = it->second;
        }

        output->updateDismanOutput(dismanOutput);
    }
    config->set_outputs(dismanOutputs);
}

std::map<int, WaylandOutput*> WaylandInterface::outputMap() const
{
    std::map<int, WaylandOutput*> ret;

    auto it = m_outputMap.cbegin();
    while (it != m_outputMap.cend()) {
        ret[it->first] = it->second;
        ++it;
    }
    return ret;
}

void WaylandInterface::tryPendingConfig()
{
    if (!m_dismanPendingConfig) {
        return;
    }
    applyConfig(m_dismanPendingConfig);
    m_dismanPendingConfig = nullptr;
}

bool WaylandInterface::applyConfig(const Disman::ConfigPtr& newConfig)
{
    return apply_config_impl(newConfig, false);
}

bool WaylandInterface::apply_config_impl(const Disman::ConfigPtr& newConfig, bool force)
{
    using namespace Wrapland::Client;

    qCDebug(DISMAN_WAYLAND) << "Applying config in wlroots backend.";

    // Create a new configuration object
    auto* wlConfig = m_outputManager->createConfiguration();
    wlConfig->setEventQueue(m_queue);

    bool changed = false;

    if (update.pending) {
        qCDebug(DISMAN_WAYLAND)
            << "Last apply still pending, remembering new changes and will apply afterwards.";
        m_dismanPendingConfig = newConfig;
        return true;
    }

    for (auto const& [key, output] : newConfig->outputs()) {
        changed |= m_outputMap[output->id()]->setWlConfig(wlConfig, output);
    }

    if (!changed && !force) {
        qCDebug(DISMAN_WAYLAND)
            << "New config equals compositor's current data. Aborting apply request.";
        return false;
    }

    // We now block changes in order to compress events while the compositor is doing its thing
    // once it's done or failed, we'll trigger config_changed() only once, and not per individual
    // property change.
    connect(wlConfig, &WlrOutputConfigurationV1::succeeded, this, [this, wlConfig] {
        qCDebug(DISMAN_WAYLAND) << "Config applied successfully.";
        wlConfig->deleteLater();
    });
    connect(wlConfig, &WlrOutputConfigurationV1::failed, this, [this, wlConfig] {
        qCWarning(DISMAN_WAYLAND) << "Applying config failed.";
        wlConfig->deleteLater();
        update.pending = false;
        Q_EMIT config_changed();
        tryPendingConfig();
    });
    connect(wlConfig, &WlrOutputConfigurationV1::cancelled, this, [this, newConfig, wlConfig] {
        // Can occur if serials were not in sync because of some simultaneous change server-side.
        // We try to apply the current config again as we should have received a done event now.
        wlConfig->deleteLater();
        update.pending = false;
        auto cfg = m_dismanPendingConfig ? m_dismanPendingConfig : newConfig;
        m_dismanPendingConfig = nullptr;
        apply_config_impl(cfg, true);
    });

    update.pending = true;
    wlConfig->apply();
    qCDebug(DISMAN_WAYLAND) << "Config sent to compositor.";
    return true;
}
