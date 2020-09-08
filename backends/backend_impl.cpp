/*
    SPDX-FileCopyrightText: 2020 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only
*/
#include "backend_impl.h"

#include "filer_controller.h"

namespace Disman
{

BackendImpl::BackendImpl()
    : Backend()
    , m_filer_controller{new Filer_controller}
{
}

BackendImpl::~BackendImpl() = default;

void BackendImpl::init([[maybe_unused]] QVariantMap const& arguments)
{
    // noop, maybe overridden in individual backends.
}

Filer_controller* BackendImpl::filer_controller() const
{
    return m_filer_controller.get();
}

Disman::ConfigPtr BackendImpl::config() const
{
    return config_impl();
}

void BackendImpl::set_config(Disman::ConfigPtr const& config)
{
    if (!config) {
        return;
    }
    set_config_impl(config);
}

}
