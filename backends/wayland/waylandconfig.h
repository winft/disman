/*************************************************************************
Copyright © 2014-2015 Sebastian Kügler <sebas@kde.org>
Copyright © 2019-2020 Roman Gilg <subdiff@gmail.com>

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

#include "abstractbackend.h"
#include "config.h"

#include <QEventLoop>
#include <QPointer>

#include <vector>

class KPluginMetaData;

namespace Disman
{
class Output;
class WaylandInterface;
class WaylandOutput;
class WaylandScreen;

/**
 * @class WaylandConfig
 *
 * This class holds the basic skeleton of the configuration and takes care of
 * fetching the information from the Wayland server and synchronizing the
 * configuration out to the "clients" that receive the config from the backend.
 * We initialize a wayland connection, using a threaded event queue when
 * querying the wayland server for data.
 * Initially, the creation of a WaylandConfig blocks until all data has been
 * received, signalled by the initialized() signal. This means that the
 * wayland client has received information about all interfaces, and that all
 * outputs are completely initialized. From then on, we properly notifyUpdate().
 */
class WaylandConfig : public QObject
{
    Q_OBJECT

public:
    explicit WaylandConfig(QObject* parent = nullptr);
    ~WaylandConfig() override;

    virtual Disman::ConfigPtr currentConfig();
    virtual QMap<int, WaylandOutput*> outputMap() const;

    void applyConfig(const Disman::ConfigPtr& newConfig);

    bool isInitialized() const;

Q_SIGNALS:
    void configChanged();
    void initialized();

private:
    struct PendingInterface {
        bool operator==(const PendingInterface& other)
        {
            return name == other.name;
        }
        QString name;
        WaylandInterface* interface;
        QThread* thread;
    };

    void initKWinTabletMode();
    void setScreenOutputs();

    void queryInterfaces();
    void queryInterface(KPluginMetaData* plugin);
    void takeInterface(const PendingInterface& pending);
    void rejectInterface(const PendingInterface& pending);

    Disman::ConfigPtr m_dismanConfig;
    WaylandScreen* m_screen;
    QPointer<WaylandInterface> m_interface;
    QThread* m_thread;

    bool m_tabletModeAvailable;
    bool m_tabletModeEngaged;

    QEventLoop m_syncLoop;

    std::vector<PendingInterface> m_pendingInterfaces;
};

}
