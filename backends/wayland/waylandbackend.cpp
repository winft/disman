/*************************************************************************************
 *  Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *  Copyright (C) 2012, 2013 by Daniel Vrátil <dvratil@redhat.com>                   *
 *  Copyright 2014-2015 Sebastian Kügler <sebas@kde.org>                             *
 *                                                                                   *
 *  This library is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU Lesser General Public                       *
 *  License as published by the Free Software Foundation; either                     *
 *  version 2.1 of the License, or (at your option) any later version.               *
 *                                                                                   *
 *  This library is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                *
 *  Lesser General Public License for more details.                                  *
 *                                                                                   *
 *  You should have received a copy of the GNU Lesser General Public                 *
 *  License along with this library; if not, write to the Free Software              *
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA       *
 *************************************************************************************/
#include "waylandbackend.h"

#include "wayland_interface.h"
#include "waylandoutput.h"
#include "waylandscreen.h"

#include "tabletmodemanager_interface.h"
#include "wayland_logging.h"

#include <config.h>
#include <configmonitor.h>
#include <generator.h>
#include <mode.h>

#include <QThread>

using namespace Disman;

WaylandBackend::WaylandBackend()
    : Disman::BackendImpl()
    , m_screen{new WaylandScreen}
{
    qCDebug(DISMAN_WAYLAND) << "Loading Wayland backend.";
    initKWinTabletMode();
    queryInterface();
}

WaylandBackend::~WaylandBackend()
{
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
    }
}

void WaylandBackend::initKWinTabletMode()
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
                if (m_interface && m_interface->is_initialized) {
                    Q_EMIT config_changed(config());
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
                if (m_interface && m_interface->is_initialized) {
                    Q_EMIT config_changed(config());
                }
            });
}

QString WaylandBackend::name() const
{
    return QStringLiteral("wayland");
}

QString WaylandBackend::service_name() const
{
    return QStringLiteral("org.kwinft.disman.backend.wayland");
}

void WaylandBackend::update_config(ConfigPtr& config) const
{
    // TODO: do this setScreen call less clunky
    config->setScreen(m_screen->toDismanScreen(config));

    m_interface->updateConfig(config);
    config->set_tablet_mode_available(m_tabletModeAvailable);
    config->set_tablet_mode_engaged(m_tabletModeEngaged);

    ScreenPtr screen = config->screen();
    m_screen->updateDismanScreen(screen);
}

bool WaylandBackend::set_config_system(Disman::ConfigPtr const& config)
{
    return m_interface->applyConfig(config);
}

std::map<int, WaylandOutput*> WaylandBackend::outputMap() const
{
    return m_interface->outputMap();
}

bool WaylandBackend::valid() const
{
    return m_interface && m_interface->is_initialized;
}

void WaylandBackend::setScreenOutputs()
{
    std::vector<WaylandOutput*> outputs;
    for (auto const& [key, out] : outputMap()) {
        outputs.push_back(out);
    }
    m_screen->setOutputs(outputs);
}

void WaylandBackend::queryInterface()
{
    QTimer::singleShot(3000, this, [this] {
        if (m_syncLoop.isRunning()) {
            qCWarning(DISMAN_WAYLAND) << "Connection to Wayland server timed out. Does the "
                                         "compositor support output management?";
            m_syncLoop.quit();
        }
    });

    m_thread = new QThread;
    m_interface = std::make_unique<WaylandInterface>(m_thread);
    connect(m_interface.get(), &WaylandInterface::connectionFailed, this, [this] {
        qCWarning(DISMAN_WAYLAND) << "Backend connection failed.";
        if (m_syncLoop.isRunning()) {
            m_syncLoop.quit();
        }
    });

    connect(m_interface.get(), &WaylandInterface::config_changed, this, [this] {
        if (handle_config_change()) {
            // When windowing system and us have been synced up quit the sync loop.
            // That returns the current config.
            m_syncLoop.quit();
        }
    });

    setScreenOutputs();
    connect(m_interface.get(),
            &WaylandInterface::outputsChanged,
            this,
            &WaylandBackend::setScreenOutputs);

    m_syncLoop.exec();
}
