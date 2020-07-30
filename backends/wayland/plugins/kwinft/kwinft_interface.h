/*************************************************************************
Copyright © 2020 Roman Gilg <subdiff@gmail.com>

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

namespace Wrapland
{
namespace Client
{
class ConnectionThread;
class EventQueue;
class Registry;
class OutputManagementV1;
}
}

namespace Disman
{
class Output;
class KwinftOutput;
class WaylandOutput;
class WaylandScreen;

class KwinftFactory : public WaylandFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kwinft.disman.waylandinterface" FILE "kwinft.json")

public:
    WaylandInterface* createInterface(QObject *parent = nullptr) override;
};

class KwinftInterface : public WaylandInterface
{
    Q_OBJECT

public:
    explicit KwinftInterface(QObject *parent = nullptr);
    ~KwinftInterface() override = default;

    void initConnection(QThread *thread) override;
    bool isInitialized() const override;

    QMap<int, WaylandOutput*> outputMap() const override;

    void applyConfig(const Disman::ConfigPtr &newConfig) override;
    void updateConfig(Disman::ConfigPtr &config) override;

protected:
    void insertOutput(WaylandOutput *output) override;
    WaylandOutput* takeOutput(WaylandOutput *output) override;
    void handleDisconnect() override;

private:
    void setupRegistry();
    void addOutputDevice(quint32 name, quint32 version);
    void tryPendingConfig();

    Wrapland::Client::ConnectionThread *m_connection;
    Wrapland::Client::EventQueue *m_queue;

    Wrapland::Client::Registry *m_registry;
    Wrapland::Client::OutputManagementV1 *m_outputManagement;

    // Wrapland names as keys
    QMap<int, KwinftOutput*> m_outputMap;

    // Wrapland names
    int m_lastOutputId = -1;

    bool m_registryInitialized;
    bool m_blockSignals;
    Disman::ConfigPtr m_dismanPendingConfig;

    int m_outputId = 0;
};

}
