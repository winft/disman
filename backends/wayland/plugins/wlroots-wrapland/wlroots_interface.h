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
class WlrOutputManagerV1;
class WlrOutputHeadV1;
}
}

namespace Disman
{
class Output;
class WlrootsOutput;
class WaylandOutput;
class WaylandScreen;

class WlrootsFactory : public WaylandFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kwinft.disman.waylandinterface" FILE "wlroots-wrapland.json")

public:
    WaylandInterface* createInterface(QObject* parent = nullptr) override;
};

class WlrootsInterface : public WaylandInterface
{
    Q_OBJECT
public:
    explicit WlrootsInterface(QObject* parent = nullptr);
    ~WlrootsInterface() override = default;

    void initConnection(QThread* thread) override;
    bool isInitialized() const override;

    std::map<int, WaylandOutput*> outputMap() const override;

    bool applyConfig(const Disman::ConfigPtr& newConfig) override;
    void updateConfig(Disman::ConfigPtr& config) override;

    Wrapland::Client::WlrOutputManagerV1* outputManager() const;

protected:
    void insertOutput(WaylandOutput* output) override;
    WaylandOutput* takeOutput(WaylandOutput* output) override;
    void handleDisconnect() override;

private:
    void setupRegistry();
    void addHead(Wrapland::Client::WlrOutputHeadV1* head);
    void tryPendingConfig();

    Wrapland::Client::ConnectionThread* m_connection;
    Wrapland::Client::EventQueue* m_queue;

    Wrapland::Client::Registry* m_registry;
    Wrapland::Client::WlrOutputManagerV1* m_outputManager;

    // Wrapland names as keys
    std::map<int, WlrootsOutput*> m_outputMap;

    // Wrapland names
    int m_lastOutputId = -1;

    bool m_registryInitialized;
    bool m_blockSignals;
    Disman::ConfigPtr m_dismanPendingConfig;

    int m_outputId = 0;
};

}
