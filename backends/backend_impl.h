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

    ConfigPtr config() const override;
    void set_config(ConfigPtr const& config) override;

protected:
    virtual ConfigPtr config_impl() const = 0;
    virtual bool set_config_impl(ConfigPtr const& config) = 0;
};

}

#endif
