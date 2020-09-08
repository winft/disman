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

#include "filer_controller.h"

#include "tabletmodemanager_interface.h"
#include "wayland_logging.h"

#include <config.h>
#include <configmonitor.h>
#include <generator.h>
#include <mode.h>

#include <KPluginLoader>
#include <KPluginMetaData>

#include <QThread>

using namespace Disman;

WaylandBackend::WaylandBackend()
    : Disman::BackendImpl()
    , m_screen{new WaylandScreen}
{
    qCDebug(DISMAN_WAYLAND) << "Loading Wayland backend.";
    initKWinTabletMode();
    queryInterfaces();
}

WaylandBackend::~WaylandBackend()
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
                if (m_interface && m_interface->isInitialized()) {
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
                if (m_interface && m_interface->isInitialized()) {
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

ConfigPtr WaylandBackend::config_impl() const
{
    // Note: This should ONLY be called from GetConfigOperation!

    ConfigPtr config(new Config);

    // TODO: do this setScreen call less clunky
    config->setScreen(m_screen->toDismanScreen(config));

    // We update from the windowing system first so the controller knows about the current
    // configuration and then update one more time so the windowing system can override values
    // it provides itself.
    m_interface->updateConfig(config);
    filer_controller()->read(config);
    m_interface->updateConfig(config);

    ScreenPtr screen = config->screen();
    m_screen->updateDismanScreen(screen);

    return config;
}

bool WaylandBackend::set_config_impl(Disman::ConfigPtr const& config)
{
    if (QLoggingCategory category("disman.wayland"); category.isEnabled(QtDebugMsg)) {
        qCDebug(DISMAN_WAYLAND) << "About to set config."
                                << "\n  Previous config:" << this->config()
                                << "\n  New config:" << config;
    }

    filer_controller()->write(config);

    for (auto const& [key, output] : config->outputs()) {
        if (auto source_id = output->replication_source()) {
            auto source = config->output(source_id);
            output->set_position(source->position());
            output->force_geometry(source->geometry());
        }
    }
    return m_interface->applyConfig(config);
}

std::map<int, WaylandOutput*> WaylandBackend::outputMap() const
{
    return m_interface->outputMap();
}

bool WaylandBackend::valid() const
{
    return m_interface && m_interface->isInitialized();
}

void WaylandBackend::setScreenOutputs()
{
    std::vector<WaylandOutput*> outputs;
    for (auto const& [key, out] : outputMap()) {
        outputs.push_back(out);
    }
    m_screen->setOutputs(outputs);
}

void WaylandBackend::queryInterfaces()
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

void WaylandBackend::queryInterface(KPluginMetaData* plugin)
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
    });

    pending.interface->initConnection(pending.thread);
}

void WaylandBackend::takeInterface(const PendingInterface& pending)
{
    m_interface = pending.interface;
    m_thread = pending.thread;
    connect(m_interface, &WaylandInterface::config_changed, this, [this] {
        auto cfg = config();
        if (!m_config || m_config->hash() != cfg->hash()) {
            qCDebug(DISMAN_WAYLAND) << "Config with new output pattern received:" << cfg;

            if (cfg->origin() == Config::Origin::unknown) {
                qCDebug(DISMAN_WAYLAND)
                    << "Config received that is unknown. Creating an optimized config now.";
                Generator generator(cfg, m_config);
                generator.optimize();
                cfg = generator.config();
            } else {
                // We set the windowing system to our saved values. They were overriden before so
                // re-read them.
                filer_controller()->read(cfg);
            }

            m_config = cfg;

            if (set_config_impl(cfg)) {
                qCDebug(DISMAN_WAYLAND) << "Config for new output pattern sent.";
                return;
            }
        }

        m_syncLoop.quit();
        Q_EMIT config_changed(cfg);
    });

    setScreenOutputs();
    connect(
        m_interface, &WaylandInterface::outputsChanged, this, &WaylandBackend::setScreenOutputs);

    qCDebug(DISMAN_WAYLAND) << "Backend" << pending.name << "initialized.";
}

void WaylandBackend::rejectInterface(const PendingInterface& pending)
{
    pending.thread->quit();
    pending.thread->wait();
    delete pending.thread;
    delete pending.interface;

    qCDebug(DISMAN_WAYLAND) << "Backend" << pending.name << "rejected.";
}
