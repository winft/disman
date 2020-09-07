/*************************************************************************
Copyright © 2014-2015   Sebastian Kügler <sebas@kde.org>
Copyright © 2019-2020   Roman Gilg <subdiff@gmail.com>

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
#pragma once

#include "config.h"
#include "wayland_interface.h"

namespace KWayland
{
namespace Client
{
class ConnectionThread;
class EventQueue;
class Registry;
class OutputManagement;
}
}

namespace Disman
{
class Output;
class KWaylandOutput;
class WaylandOutput;
class WaylandScreen;

class KWaylandFactory : public WaylandFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kwinft.disman.waylandinterface" FILE "kwayland.json")

public:
    WaylandInterface* createInterface(QObject* parent = nullptr) override;
};

class KWaylandInterface : public WaylandInterface
{
    Q_OBJECT

public:
    explicit KWaylandInterface(QObject* parent = nullptr);
    ~KWaylandInterface() override = default;
    bool isInitialized() const override;

    std::map<int, WaylandOutput*> outputMap() const override;

    bool applyConfig(const Disman::ConfigPtr& newConfig) override;
    void updateConfig(Disman::ConfigPtr& config) override;

protected:
    void initConnection(QThread* thread) override;
    void insertOutput(WaylandOutput* output) override;
    WaylandOutput* takeOutput(WaylandOutput* output) override;
    void handleDisconnect() override;

private:
    void setupRegistry();
    void addOutputDevice(quint32 name, quint32 version);
    void tryPendingConfig();

    KWayland::Client::ConnectionThread* m_connection;
    KWayland::Client::EventQueue* m_queue;

    KWayland::Client::Registry* m_registry;
    KWayland::Client::OutputManagement* m_outputManagement;

    // KWayland names as keys
    std::map<int, KWaylandOutput*> m_outputMap;

    // KWayland names
    int m_lastOutputId = -1;

    bool m_registryInitialized;
    bool m_blockSignals;
    Disman::ConfigPtr m_dismanPendingConfig;
};

}
