/*
    SPDX-FileCopyrightText: 2020 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only
*/
#include "watcher.h"
#include "doctor.h"

#include "configmonitor.h"

static QTextStream cout(stdout);
static QTextStream cerr(stderr);

namespace Disman::Ctl
{

Watcher::Watcher(Disman::ConfigPtr config, QObject* parent)
    : QObject(parent)
    , m_config{config}
{
    Disman::ConfigMonitor::instance()->add_config(m_config);
    connect(Disman::ConfigMonitor::instance(),
            &Disman::ConfigMonitor::configuration_changed,
            this,
            &Watcher::changed,
            Qt::UniqueConnection);

    cout << "-- Current config --" << Qt::endl;
    Doctor::showOutputs(m_config);
}

void Watcher::changed()
{
    cout << "\n-- Config change " << ++m_count << " --" << Qt::endl;
    Doctor::showOutputs(m_config);
}

}
