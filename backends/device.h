/*
    SPDX-FileCopyrightText: 2020 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only
*/
#ifndef DEVICE_H
#define DEVICE_H

#include "types.h"

#include <QObject>
#include <QString>
#include <memory>

class OrgFreedesktopDBusPropertiesInterface;
class QDBusPendingCallWatcher;
class QDBusInterface;
class QTimer;

namespace Disman
{

class Device : public QObject
{
    Q_OBJECT

public:
    explicit Device(QObject* parent = nullptr);
    ~Device();

    bool lid_present() const;
    bool lid_open() const;

Q_SIGNALS:
    void lid_open_changed();

private Q_SLOTS:
    void fetch_lid_closed();
    void prepare_for_sleep(bool start);

private:
    void set_ready();

    void fetch_lid_present();
    void lid_present_fetched(QDBusPendingCallWatcher* watcher);

    void lid_closed_fetched(QDBusPendingCallWatcher* watcher);

    bool m_ready{false};
    bool m_lid_present{false};
    bool m_lid_closed{false};

    std::unique_ptr<QTimer> m_lid_timer;

    OrgFreedesktopDBusPropertiesInterface* m_upower{nullptr};
    QDBusInterface* m_login1{nullptr};
};

}

#endif
