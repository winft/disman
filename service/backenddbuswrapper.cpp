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
#include "backenddbuswrapper.h"
#include "backendadaptor.h"
#include "backendloader.h"
#include "disman_backend_launcher_debug.h"

#include "backend.h"
#include "config.h"
#include "configserializer_p.h"

#include <QDBusConnection>
#include <QDBusError>

BackendDBusWrapper::BackendDBusWrapper(Disman::Backend* backend)
    : QObject()
    , mBackend(backend)
{
    connect(mBackend,
            &Disman::Backend::config_changed,
            this,
            &BackendDBusWrapper::backendConfigChanged);

    mChangeCollector.setSingleShot(true);
    mChangeCollector.setInterval(200); // wait for 200 msecs without any change
                                       // before actually emitting configChanged
    connect(&mChangeCollector, &QTimer::timeout, this, &BackendDBusWrapper::doEmitConfigChanged);
}

BackendDBusWrapper::~BackendDBusWrapper()
{
}

bool BackendDBusWrapper::init()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    new BackendAdaptor(this);
    if (!dbus.registerObject(QStringLiteral("/backend"), this, QDBusConnection::ExportAdaptors)) {
        qCWarning(DISMAN_BACKEND_LAUNCHER)
            << "Failed to export backend to DBus: another launcher already running?";
        qCWarning(DISMAN_BACKEND_LAUNCHER) << dbus.lastError().message();
        return false;
    }

    return true;
}

QVariantMap BackendDBusWrapper::getConfig() const
{
    auto const config = mBackend->config();
    assert(config != nullptr);
    if (!config) {
        qCWarning(DISMAN_BACKEND_LAUNCHER) << "Backend provided an empty config!";
        return QVariantMap();
    }

    const QJsonObject obj = Disman::ConfigSerializer::serialize_config(mBackend->config());
    Q_ASSERT(!obj.isEmpty());
    return obj.toVariantMap();
}

QVariantMap BackendDBusWrapper::setConfig(const QVariantMap& configMap)
{
    if (configMap.isEmpty()) {
        qCWarning(DISMAN_BACKEND_LAUNCHER) << "Received an empty config map";
        return QVariantMap();
    }

    const Disman::ConfigPtr config = Disman::ConfigSerializer::deserialize_config(configMap);
    mBackend->set_config(config);

    mCurrentConfig = mBackend->config();
    QMetaObject::invokeMethod(this, "doEmitConfigChanged", Qt::QueuedConnection);

    // TODO: set_config should return adjusted config that was actually applied
    const QJsonObject obj = Disman::ConfigSerializer::serialize_config(mCurrentConfig);
    Q_ASSERT(!obj.isEmpty());
    return obj.toVariantMap();
}

void BackendDBusWrapper::backendConfigChanged(const Disman::ConfigPtr& config)
{
    assert(config != nullptr);
    if (!config) {
        qCWarning(DISMAN_BACKEND_LAUNCHER) << "Backend provided an empty config!";
        return;
    }

    mCurrentConfig = config;
    mChangeCollector.start();
}

void BackendDBusWrapper::doEmitConfigChanged()
{
    assert(mCurrentConfig != nullptr);
    if (!mCurrentConfig) {
        return;
    }

    const QJsonObject obj = Disman::ConfigSerializer::serialize_config(mCurrentConfig);
    Q_EMIT configChanged(obj.toVariantMap());

    mCurrentConfig.reset();
    mChangeCollector.stop();
}
