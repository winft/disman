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

    void initConnection(QThread* thread);
    bool isInitialized() const;

    std::map<int, WaylandOutput*> outputMap() const;

    bool applyConfig(const Disman::ConfigPtr& newConfig);
    void updateConfig(Disman::ConfigPtr& config);

    Wrapland::Client::WlrOutputManagerV1* outputManager() const;

Q_SIGNALS:
    void config_changed();
    void initialized();
    void connectionFailed(const QString& socketName);
    void outputsChanged();

private:
    void checkInitialized();

    /**
     * Finalize: when the output is is initialized, we put it in the known outputs map,
     * remove it from the list of initializing outputs, and emit config_changed().
     */
    void initOutput(WaylandOutput* output);

    void insertOutput(WaylandOutput* output);
    void addOutput(WaylandOutput* output);
    void removeOutput(WaylandOutput* output);
    void handleDisconnect();

    void blockSignals();
    void unblockSignals();

    void setupRegistry();
    void addHead(Wrapland::Client::WlrOutputHeadV1* head);

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

    bool m_blockSignals{true};
    Disman::ConfigPtr m_dismanPendingConfig{nullptr};

    int m_outputId = 0;

    // Compositor side names
    QList<WaylandOutput*> m_initializingOutputs;

    Disman::ConfigPtr m_dismanConfig;
    WaylandScreen* m_screen;
};

}
