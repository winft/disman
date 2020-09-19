/*
    SPDX-FileCopyrightText: 2020 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only
*/
#ifndef BACKEND_IMPL_H
#define BACKEND_IMPL_H

#include "backend.h"

#include <memory>

namespace Disman
{
class Device;
class Filer_controller;

class BackendImpl : public Backend
{
    Q_OBJECT

public:
    explicit BackendImpl();
    ~BackendImpl();

    void init(const QVariantMap& arguments) override;

    ConfigPtr config() const override;
    void set_config(ConfigPtr const& config) override;

protected:
    Filer_controller* filer_controller() const;

    virtual void update_config(ConfigPtr& config) const = 0;
    virtual bool set_config_impl(ConfigPtr const& config) = 0;

    /**
     * Handles a change in the window system. Sets a stored or generated config if needed and
     * returns false in this case. If the state correspondes to the stored state or is already
     * optimal returns true.
     */
    bool handle_config_change();

private:
    void load_lid_config();

    std::unique_ptr<Device> m_device;
    std::unique_ptr<Filer_controller> m_filer_controller;

    mutable bool m_config_initialized{false};

    ConfigPtr m_config;
};

}

#endif
