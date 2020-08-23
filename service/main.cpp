/*
 * Copyright (C) 2014  Daniel Vratil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#include <QDBusConnection>
#include <QGuiApplication>
#include <QSessionManager>

#include "backendloader.h"
#include "disman_backend_launcher_debug.h"
#include "log.h"

int main(int argc, char** argv)
{
    Disman::Log::instance();
    QGuiApplication::setDesktopSettingsAware(false);
    QGuiApplication app(argc, argv);

    auto disableSessionManagement
        = [](QSessionManager& sm) { sm.setRestartHint(QSessionManager::RestartNever); };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    if (!QDBusConnection::sessionBus().registerService(QStringLiteral("org.kwinft.disman"))) {
        qCWarning(DISMAN_BACKEND_LAUNCHER)
            << "Cannot register Disman service. Another launcher already running?";
        return -1;
    }

    BackendLoader* loader = new BackendLoader;
    if (!loader->init()) {
        return -2;
    }

    const int ret = app.exec();

    // Make sure the backend is destroyed and unloaded before we return (i.e.
    // as long as QApplication object and it's XCB connection still exist
    delete loader;

    return ret;
}