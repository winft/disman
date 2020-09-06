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
#include "kwinft_interface.h"

#include "kwinft_logging.h"
#include "kwinft_output.h"

#include <Wrapland/Client/connection_thread.h>
#include <Wrapland/Client/event_queue.h>
#include <Wrapland/Client/output_configuration_v1.h>
#include <Wrapland/Client/output_management_v1.h>
#include <Wrapland/Client/registry.h>

#include <QThread>

using namespace Disman;

WaylandInterface* KwinftFactory::createInterface(QObject* parent)
{
    return new KwinftInterface(parent);
}

KwinftInterface::KwinftInterface(QObject* parent)
    : WaylandInterface(parent)
    , m_outputManagement(nullptr)
    , m_registryInitialized(false)
    , m_dismanPendingConfig(nullptr)
{
}

void KwinftInterface::initConnection(QThread* thread)
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

bool KwinftInterface::isInitialized() const
{
    return m_registryInitialized && m_outputManagement != nullptr
        && WaylandInterface::isInitialized();
}

void KwinftInterface::handleDisconnect()
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

void KwinftInterface::setupRegistry()
{
    m_queue = new Wrapland::Client::EventQueue(this);
    m_queue->setup(m_connection);

    m_registry = new Wrapland::Client::Registry(this);

    connect(m_registry,
            &Wrapland::Client::Registry::outputDeviceV1Announced,
            this,
            &KwinftInterface::addOutputDevice);

    connect(m_registry,
            &Wrapland::Client::Registry::outputManagementV1Announced,
            this,
            [this](quint32 name, quint32 version) {
                m_outputManagement
                    = m_registry->createOutputManagementV1(name, version, m_registry);
                m_outputManagement->setEventQueue(m_queue);
            });

    connect(m_registry, &Wrapland::Client::Registry::interfacesAnnounced, this, [this] {
        m_registryInitialized = true;
        unblockSignals();
        checkInitialized();
    });

    m_registry->create(m_connection);
    m_registry->setEventQueue(m_queue);
    m_registry->setup();
}

void KwinftInterface::addOutputDevice(quint32 name, quint32 version)
{
    auto output = new KwinftOutput(++m_outputId, this);
    output->createOutputDevice(m_registry, name, version);
    addOutput(output);
}

void KwinftInterface::insertOutput(WaylandOutput* output)
{
    auto* out = static_cast<KwinftOutput*>(output);
    m_outputMap.insert(out->id(), out);
}

WaylandOutput* KwinftInterface::takeOutput(WaylandOutput* output)
{
    auto* out = static_cast<KwinftOutput*>(output);
    return m_outputMap.take(out->id());
}

void KwinftInterface::updateConfig(Disman::ConfigPtr& config)
{
    config->set_supported_features(Config::Feature::Writable | Config::Feature::PerOutputScaling
                                   | Config::Feature::OutputReplication
                                   | Config::Feature::AutoRotation | Config::Feature::TabletMode);
    config->set_valid(m_connection->display());

    // Removing removed outputs
    const Disman::OutputList outputs = config->outputs();
    for (const auto& output : outputs) {
        if (!m_outputMap.contains(output->id())) {
            config->remove_output(output->id());
        }
    }

    // Add Disman::Outputs that aren't in the list yet.
    Disman::OutputList dismanOutputs = config->outputs();
    for (const auto& output : m_outputMap) {
        Disman::OutputPtr dismanOutput = dismanOutputs[output->id()];
        if (!dismanOutput) {
            dismanOutput = output->toDismanOutput();
            dismanOutputs.insert(dismanOutput->id(), dismanOutput);
        }
        output->updateDismanOutput(dismanOutput);
    }
    config->set_outputs(dismanOutputs);
}

QMap<int, WaylandOutput*> KwinftInterface::outputMap() const
{
    QMap<int, WaylandOutput*> ret;

    auto it = m_outputMap.constBegin();
    while (it != m_outputMap.constEnd()) {
        ret[it.key()] = it.value();
        ++it;
    }
    return ret;
}

void KwinftInterface::tryPendingConfig()
{
    if (!m_dismanPendingConfig) {
        return;
    }
    applyConfig(m_dismanPendingConfig);
    m_dismanPendingConfig = nullptr;
}

bool KwinftInterface::applyConfig(const Disman::ConfigPtr& newConfig)
{
    using namespace Wrapland::Client;

    qCDebug(DISMAN_WAYLAND) << "Applying config in KWinFT backend.";

    // Create a new configuration object
    auto* wlConfig = m_outputManagement->createConfiguration();
    wlConfig->setEventQueue(m_queue);

    bool changed = false;

    if (signalsBlocked()) {
        qCDebug(DISMAN_WAYLAND)
            << "Last apply still pending, remembering new changes and will apply afterwards.";
        m_dismanPendingConfig = newConfig;
        return false;
    }

    for (const auto& output : newConfig->outputs()) {
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
    connect(wlConfig, &OutputConfigurationV1::applied, this, [this, wlConfig] {
        qCDebug(DISMAN_WAYLAND) << "Config applied successfully.";
        wlConfig->deleteLater();
        unblockSignals();
        Q_EMIT config_changed();
        tryPendingConfig();
    });
    connect(wlConfig, &OutputConfigurationV1::failed, this, [this, wlConfig] {
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
