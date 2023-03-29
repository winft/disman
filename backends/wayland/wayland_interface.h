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
#pragma once

#include <config.h>

#include <QEventLoop>
#include <QObject>
#include <QVector>

class QThread;

namespace Wrapland::Client
{
class ConnectionThread;
class EventQueue;
class Registry;
class WlrOutputManagerV1;
class WlrOutputHeadV1;
}

namespace Disman
{

class Output;
class WaylandOutput;
class WaylandScreen;

class WaylandInterface : public QObject
{
    Q_OBJECT
public:
    explicit WaylandInterface(QThread* thread);

    std::map<int, WaylandOutput*> outputMap() const;

    bool applyConfig(const Disman::ConfigPtr& newConfig);
    void updateConfig(Disman::ConfigPtr& config);

    bool is_initialized{false};

Q_SIGNALS:
    void config_changed();
    void initialized();
    void connectionFailed(const QString& socketName);
    void outputsChanged();

private:
    void handle_wlr_manager_done();

    void add_output(Wrapland::Client::WlrOutputHeadV1* head);
    void removeOutput(WaylandOutput* output);
    void handleDisconnect();

    void setupRegistry();

    bool apply_config_impl(const Disman::ConfigPtr& newConfig, bool force);
    void tryPendingConfig();

    Wrapland::Client::ConnectionThread* m_connection{nullptr};
    Wrapland::Client::EventQueue* m_queue{nullptr};

    Wrapland::Client::Registry* m_registry{nullptr};
    Wrapland::Client::WlrOutputManagerV1* m_outputManager{nullptr};

    // Wrapland names as keys
    std::map<int, WaylandOutput*> m_outputMap;

    // Wrapland names
    int m_lastOutputId = -1;

    struct {
        bool pending{true};
        bool outputs{false};
    } update;

    Disman::ConfigPtr m_dismanPendingConfig{nullptr};

    int m_outputId = 0;

    Disman::ConfigPtr m_dismanConfig;
    WaylandScreen* m_screen;
};

}
