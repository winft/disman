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
#include "wlroots_interface.h"

#include "wlroots_logging.h"
#include "wlroots_output.h"

#include <Wrapland/Client/connection_thread.h>
#include <Wrapland/Client/event_queue.h>
#include <Wrapland/Client/registry.h>
#include <Wrapland/Client/wlr_output_configuration_v1.h>
#include <Wrapland/Client/wlr_output_manager_v1.h>

#include <QThread>

using namespace Disman;

WaylandInterface* WlrootsFactory::createInterface(QObject* parent)
{
    return new WlrootsInterface(parent);
}

WlrootsInterface::WlrootsInterface(QObject* parent)
    : WaylandInterface(parent)
    , m_outputManager(nullptr)
    , m_registryInitialized(false)
    , m_dismanPendingConfig(nullptr)
{
}

void WlrootsInterface::initConnection(QThread* thread)
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

Wrapland::Client::WlrOutputManagerV1* WlrootsInterface::outputManager() const
{
    return m_outputManager;
}

bool WlrootsInterface::isInitialized() const
{
    return m_registryInitialized && m_outputManager != nullptr && WaylandInterface::isInitialized();
}

void WlrootsInterface::handleDisconnect()
{
    qDeleteAll(m_outputMap);
    m_outputMap.clear();

    // Clean up
    if (m_queue) {
        delete m_queue;
        m_queue = nullptr;
    }

    m_connection->deleteLater();
    m_connection = nullptr;

    WaylandInterface::handleDisconnect();
}

void WlrootsInterface::setupRegistry()
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
                        &WlrootsInterface::addHead);

                connect(m_outputManager, &Wrapland::Client::WlrOutputManagerV1::done, this, [this] {
                    // We only need to process this once in the beginning.
                    disconnect(m_outputManager,
                               &Wrapland::Client::WlrOutputManagerV1::done,
                               this,
                               nullptr);
                    unblockSignals();
                    checkInitialized();
                });
                m_outputManager->setEventQueue(m_queue);
            });

    connect(m_registry, &Wrapland::Client::Registry::interfacesAnnounced, this, [this] {
        m_registryInitialized = true;
        checkInitialized();
    });

    m_registry->setEventQueue(m_queue);
    m_registry->create(m_connection);
    m_registry->setup();
}

void WlrootsInterface::addHead(Wrapland::Client::WlrOutputHeadV1* head)
{
    auto output = new WlrootsOutput(++m_outputId, head, this);
    addOutput(output);
}

void WlrootsInterface::insertOutput(WaylandOutput* output)
{
    auto* out = static_cast<WlrootsOutput*>(output);
    m_outputMap.insert(out->id(), out);
}

WaylandOutput* WlrootsInterface::takeOutput(WaylandOutput* output)
{
    auto* out = static_cast<WlrootsOutput*>(output);
    return m_outputMap.take(out->id());
}

void WlrootsInterface::updateConfig(Disman::ConfigPtr& config)
{
    config->set_supported_features(Config::Feature::Writable | Config::Feature::PerOutputScaling);
    config->set_valid(m_connection->display());

    // Removing removed outputs
    const Disman::OutputList outputs = config->outputs();
    for (auto const& [key, output] : outputs) {
        if (!m_outputMap.contains(output->id())) {
            config->remove_output(output->id());
        }
    }

    // Add Disman::Outputs that aren't in the list yet.
    Disman::OutputList dismanOutputs = config->outputs();

    for (const auto& output : m_outputMap) {
        Disman::OutputPtr dismanOutput;

        auto it = dismanOutputs.find(output->id());
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

QMap<int, WaylandOutput*> WlrootsInterface::outputMap() const
{
    QMap<int, WaylandOutput*> ret;

    auto it = m_outputMap.constBegin();
    while (it != m_outputMap.constEnd()) {
        ret[it.key()] = it.value();
        ++it;
    }
    return ret;
}

void WlrootsInterface::tryPendingConfig()
{
    if (!m_dismanPendingConfig) {
        return;
    }
    applyConfig(m_dismanPendingConfig);
    m_dismanPendingConfig = nullptr;
}

bool WlrootsInterface::applyConfig(const Disman::ConfigPtr& newConfig)
{
    using namespace Wrapland::Client;

    qCDebug(DISMAN_WAYLAND) << "Applying config in wlroots backend.";

    // Create a new configuration object
    auto* wlConfig = m_outputManager->createConfiguration();
    wlConfig->setEventQueue(m_queue);

    bool changed = false;

    if (signalsBlocked()) {
        qCDebug(DISMAN_WAYLAND)
            << "Last apply still pending, remembering new changes and will apply afterwards.";
        m_dismanPendingConfig = newConfig;
        return false;
    }

    for (auto const& [key, output] : newConfig->outputs()) {
        changed |= m_outputMap[output->id()]->setWlConfig(wlConfig, output);
    }

    if (!changed) {
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
        unblockSignals();
        Q_EMIT config_changed();
        tryPendingConfig();
    });
    connect(wlConfig, &WlrOutputConfigurationV1::failed, this, [this, wlConfig] {
        qCWarning(DISMAN_WAYLAND) << "Applying config failed.";
        wlConfig->deleteLater();
        unblockSignals();
        Q_EMIT config_changed();
        tryPendingConfig();
    });
    connect(wlConfig, &WlrOutputConfigurationV1::cancelled, this, [this, wlConfig] {
        // This should never be received since we apply the new config directly. But in case we just
        // do the same as on failed.
        qCWarning(DISMAN_WAYLAND) << "Applying config failed.";
        wlConfig->deleteLater();
        unblockSignals();
        Q_EMIT config_changed();
        tryPendingConfig();
    });

    blockSignals();
    wlConfig->apply();
    qCDebug(DISMAN_WAYLAND) << "Config sent to compositor.";
    return true;
}
