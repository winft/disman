/*
    SPDX-FileCopyrightText: 2020 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only
*/
#include "device.h"

#include "freedesktop_interface.h"
#include "logging.h"

#include <QDBusConnection>

namespace Disman
{

Device::Device(QObject* parent)
    : QObject(parent)
    , m_lid_timer{new QTimer}
{
    m_lid_timer->setInterval(1000);
    m_lid_timer->setSingleShot(true);
    connect(m_lid_timer.get(), &QTimer::timeout, this, &Device::lid_open_changed);

    m_upower = new OrgFreedesktopDBusPropertiesInterface(QStringLiteral("org.freedesktop.UPower"),
                                                         QStringLiteral("/org/freedesktop/UPower"),
                                                         QDBusConnection::systemBus(),
                                                         this);
    if (!m_upower->isValid()) {
        qCDebug(DISMAN_BACKEND) << "UPower not available, no lid detection."
                                << m_upower->lastError().message();
        return;
    }

    QDBusConnection::systemBus().connect(QStringLiteral("org.freedesktop.UPower"),
                                         QStringLiteral("/org/freedesktop/UPower"),
                                         QStringLiteral("org.freedesktop.DBus.Properties"),
                                         QStringLiteral("PropertiesChanged"),
                                         this,
                                         SLOT(fetch_lid_closed()));

    m_login1 = new QDBusInterface(QStringLiteral("org.freedesktop.login1"),
                                  QStringLiteral("/org/freedesktop/login1"),
                                  QStringLiteral("org.freedesktop.login1.Manager"),
                                  QDBusConnection::systemBus(),
                                  this);
    if (!m_login1->isValid()) {
        qCDebug(DISMAN_BACKEND) << "logind not available, no lid detection."
                                << m_login1->lastError().message();
        return;
    }

    connect(m_login1, SIGNAL(PrepareForSleep(bool)), this, SLOT(prepare_for_sleep(bool)));
    fetch_lid_present();
}

Device::~Device() = default;

bool Device::lid_present() const
{
    return m_lid_present;
}

bool Device::lid_open() const
{
    return !m_lid_closed;
}

void Device::fetch_lid_present()
{
    auto res
        = m_upower->Get(QStringLiteral("org.freedesktop.UPower"), QStringLiteral("LidIsPresent"));

    connect(new QDBusPendingCallWatcher(res),
            &QDBusPendingCallWatcher::finished,
            this,
            &Device::lid_present_fetched);
}

void Device::lid_present_fetched(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QVariant> const reply = *watcher;
    if (reply.isError()) {
        qCDebug(DISMAN_BACKEND) << "Error when fetching lid information: "
                                << reply.error().message();
        return;
    }

    m_lid_present = reply.value().toBool();
    watcher->deleteLater();

    if (m_lid_present) {
        fetch_lid_closed();
    }
    m_ready = true;
}

void Device::fetch_lid_closed()
{
    auto res
        = m_upower->Get(QStringLiteral("org.freedesktop.UPower"), QStringLiteral("LidIsClosed"));

    connect(new QDBusPendingCallWatcher(res),
            &QDBusPendingCallWatcher::finished,
            this,
            &Device::lid_closed_fetched);
}

void Device::lid_closed_fetched(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QVariant> const reply = *watcher;
    if (reply.isError()) {
        qCDebug(DISMAN_BACKEND) << "Error when fetching lid closed: " << reply.error().message();
        return;
    }

    auto const closed = reply.value().toBool();
    watcher->deleteLater();

    if (closed == m_lid_closed) {
        return;
    }

    m_lid_closed = closed;
    if (m_ready && !closed) {
        Q_EMIT lid_open_changed();
    } else {
        m_lid_timer->start();
    }
}

void Device::prepare_for_sleep(bool start)
{
    qCDebug(DISMAN_BACKEND) << "Device sleep change:" << (start ? "going to sleep" : "waking up");
    if (start) {
        m_lid_timer->stop();
    }
    // TODO: On start being false should we query the backend for changes in between? KDisplay
    //       daemon does that.
}

}
