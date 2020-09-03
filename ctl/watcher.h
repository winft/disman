/*
    SPDX-FileCopyrightText: 2020 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only
*/
#ifndef DISMAN_WATCHER_H
#define DISMAN_WATCHER_H

#include "output.h"
#include "types.h"

#include <QObject>

namespace Disman
{
class ConfigOperation;

namespace Ctl
{

class Watcher : public QObject
{
    Q_OBJECT

public:
    explicit Watcher(Disman::ConfigPtr config, QObject* parent = nullptr);

private:
    void changed();

    Disman::ConfigPtr m_config;
    int m_count{0};
};

}
}

#endif
