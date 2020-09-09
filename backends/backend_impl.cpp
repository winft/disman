/*
    SPDX-FileCopyrightText: 2020 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only
*/
#include "backend_impl.h"

#include "device.h"
#include "filer_controller.h"
#include "generator.h"
#include "logging.h"

namespace Disman
{

BackendImpl::BackendImpl()
    : Backend()
    , m_device{new Device}
    , m_filer_controller{new Filer_controller(m_device.get())}
{
    connect(m_device.get(), &Device::lid_open_changed, this, &BackendImpl::load_lid_config);
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
    m_config_initialized = true;
    return config_impl();
}

void BackendImpl::set_config(Disman::ConfigPtr const& config)
{
    if (!config) {
        return;
    }
    set_config_impl(config);
}

void BackendImpl::load_lid_config()
{
    if (!m_config_initialized) {
        qCWarning(DISMAN_BACKEND) << "Lid open state changed but first config has not yet been "
                                     "initialized. Doing nothing.";
        return;
    }
    auto cfg = config();

    if (m_device->lid_open()) {
        // The lid has been opnened. Try to load the open lid file.
        if (!m_filer_controller->load_lid_file(cfg)) {
            return;
        }
        qCDebug(DISMAN_BACKEND) << "Loaded lid-open file on lid being opened.";
    } else {
        // The lid has been closed, we write the current config as open-lid-config and then generate
        // an optimized one with the embedded display disabled that gets applied.
        Generator generator(cfg);
        generator.set_derived();
        qCDebug(DISMAN_BACKEND) << "Lid closed, trying to disable embedded display.";

        if (!generator.disable_embedded()) {
            // Alternative config could not be generated.
            qCWarning(DISMAN_BACKEND) << "Embedded display could not be disabled.";
            return;
        }
        if (m_filer_controller->save_lid_file(cfg)) {
            qCWarning(DISMAN_BACKEND) << "Failed to save open-lid file.";
            return;
        }
        cfg = generator.config();
    }

    set_config_impl(cfg);
}

}
