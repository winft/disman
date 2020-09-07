/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
    SPDX-FileCopyrightText: 2020 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only
*/
#include "backend.h"

namespace Disman
{

Backend::Backend(QObject* parent)
    : QObject(parent)
{
}

void Backend::init(const QVariantMap& arguments)
{
    Q_UNUSED(arguments);
}

}
