/*
    SPDX-FileCopyrightText: 2020 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only
*/
#ifndef BACKEND_IMPL_H
#define BACKEND_IMPL_H

#include "backend.h"

namespace Disman
{

class BackendImpl : public Backend
{
    Q_OBJECT

public:
    explicit BackendImpl();
    void init(const QVariantMap& arguments) override;
};

}

#endif
