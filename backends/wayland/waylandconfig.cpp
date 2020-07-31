/*************************************************************************
Copyright © 2013        Martin Gräßlin <mgraesslin@kde.org>
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
#include "waylandconfig.h"

#include "wayland_interface.h"
#include "waylandbackend.h"
#include "waylandoutput.h"
#include "waylandscreen.h"

#include "tabletmodemanager_interface.h"
#include "wayland_logging.h"

#include <KPluginLoader>
#include <KPluginMetaData>

namespace Disman
{

WaylandConfig::WaylandConfig(QObject* parent)
    : QObject(parent)
    , m_dismanConfig(new Config)
    , m_screen(new WaylandScreen(this))
    , m_tabletModeAvailable(false)
    , m_tabletModeEngaged(false)
{
    initKWinTabletMode();
    queryInterfaces();
}

WaylandConfig::~WaylandConfig()
{
    for (auto pending : m_pendingInterfaces) {
        rejectInterface(pending);
    }
    m_pendingInterfaces.clear();

    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
    }
}

void WaylandConfig::initKWinTabletMode()
{
    auto* interface = new OrgKdeKWinTabletModeManagerInterface(QStringLiteral("org.kde.KWin"),
                                                               QStringLiteral("/org/kde/KWin"),
                                                               QDBusConnection::sessionBus(),
                                                               this);
    if (!interface->isValid()) {
        m_tabletModeAvailable = false;
        m_tabletModeEngaged = false;
        return;
    }

    m_tabletModeAvailable = interface->tabletModeAvailable();
    m_tabletModeEngaged = interface->tabletMode();

    connect(interface,
            &OrgKdeKWinTabletModeManagerInterface::tabletModeChanged,
            this,
            [this](bool tabletMode) {
                if (m_tabletModeEngaged == tabletMode) {
                    return;
                }
                m_tabletModeEngaged = tabletMode;
                if (m_interface && m_interface->isInitialized()) {
                    Q_EMIT configChanged();
                }
            });
    connect(interface,
            &OrgKdeKWinTabletModeManagerInterface::tabletModeAvailableChanged,
            this,
            [this](bool available) {
                if (m_tabletModeAvailable == available) {
                    return;
                }
                m_tabletModeAvailable = available;
                if (m_interface && m_interface->isInitialized()) {
                    Q_EMIT configChanged();
                }
            });
}

bool WaylandConfig::isInitialized() const
{
    return m_interface && m_interface->isInitialized();
}

ConfigPtr WaylandConfig::currentConfig()
{
    // TODO: do this setScreen call less clunky
    m_dismanConfig->setScreen(m_screen->toDismanScreen(m_dismanConfig));

    m_interface->updateConfig(m_dismanConfig);

    ScreenPtr screen = m_dismanConfig->screen();
    m_screen->updateDismanScreen(screen);

    return m_dismanConfig;
}

void WaylandConfig::setScreenOutputs()
{
    m_screen->setOutputs(outputMap().values());
}

QMap<int, WaylandOutput*> WaylandConfig::outputMap() const
{
    return m_interface->outputMap();
}

void WaylandConfig::applyConfig(const ConfigPtr& newConfig)
{
    m_interface->applyConfig(newConfig);
}

void WaylandConfig::queryInterfaces()
{
    QTimer::singleShot(3000, this, [this] {
        for (auto pending : m_pendingInterfaces) {
            qCWarning(DISMAN_WAYLAND) << pending.name << "backend could not be aquired in time.";
            rejectInterface(pending);
        }
        if (m_syncLoop.isRunning()) {
            qCWarning(DISMAN_WAYLAND) << "Connection to Wayland server timed out. Does the "
                                         "compositor support output management?";
            m_syncLoop.quit();
        }
        m_pendingInterfaces.clear();
    });

    auto availableInterfacePlugins = KPluginLoader::findPlugins(QStringLiteral("disman/wayland"));

    for (auto plugin : availableInterfacePlugins) {
        queryInterface(&plugin);
    }
    m_syncLoop.exec();
}

void WaylandConfig::queryInterface(KPluginMetaData* plugin)
{
    PendingInterface pending;

    pending.name = plugin->name();

    for (auto other : m_pendingInterfaces) {
        if (pending.name == other.name) {
            // Names must be unique.
            return;
        }
    }

    // TODO: qobject_cast not working here. Why?
    auto* factory = dynamic_cast<WaylandFactory*>(plugin->instantiate());
    if (!factory) {
        return;
    }
    pending.interface = factory->createInterface();
    pending.thread = new QThread();

    m_pendingInterfaces.push_back(pending);
    connect(pending.interface, &WaylandInterface::connectionFailed, this, [this, &pending] {
        qCWarning(DISMAN_WAYLAND) << "Backend" << pending.name << "failed.";
        rejectInterface(pending);
        m_pendingInterfaces.erase(
            std::remove(m_pendingInterfaces.begin(), m_pendingInterfaces.end(), pending),
            m_pendingInterfaces.end());
    });

    connect(pending.interface, &WaylandInterface::initialized, this, [this, pending] {
        if (m_interface) {
            // Too late. Already have an interface initialized.
            return;
        }

        for (auto other : m_pendingInterfaces) {
            if (other.interface != pending.interface) {
                rejectInterface(other);
            }
        }
        m_pendingInterfaces.clear();

        takeInterface(pending);
        m_syncLoop.quit();
    });

    pending.interface->initConnection(pending.thread);
}

void WaylandConfig::takeInterface(const PendingInterface& pending)
{
    m_interface = pending.interface;
    m_thread = pending.thread;
    connect(m_interface, &WaylandInterface::configChanged, this, &WaylandConfig::configChanged);

    setScreenOutputs();
    connect(m_interface, &WaylandInterface::outputsChanged, this, &WaylandConfig::setScreenOutputs);

    qCDebug(DISMAN_WAYLAND) << "Backend" << pending.name << "initialized.";
    Q_EMIT initialized();
}

void WaylandConfig::rejectInterface(const PendingInterface& pending)
{
    pending.thread->quit();
    pending.thread->wait();
    delete pending.thread;
    delete pending.interface;

    qCDebug(DISMAN_WAYLAND) << "Backend" << pending.name << "rejected.";
}

}
