/*************************************************************************
Copyright © 2020   Roman Gilg <subdiff@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
**************************************************************************/
#include "filer_controller.h"

#include "device.h"
#include "filer.h"
#include "logging.h"

namespace Disman
{

Filer_controller::Filer_controller(Device* device, QObject* parent)
    : QObject(parent)
    , m_device{device}
{
}

Filer_controller::~Filer_controller() = default;

bool Filer_controller::read(ConfigPtr& config)
{
    if (!m_filer || m_filer->config()->hash() != config->hash()) {
        if (lid_file_exists(config) && m_device->lid_present() && m_device->lid_open()) {
            // Can happen when while lid closed output combination changes or device is shut down.
            move_lid_file(config);
        }

        // Different output combination means different combination of filers.
        reset_filer(config);
    }

    auto const success = m_filer->get_values(config);
    if (success) {
        config->set_cause(Config::Cause::file);
    }
    return success;
}

bool Filer_controller::write(ConfigPtr const& config)
{
    if (m_filer) {
        if (m_filer->config()->hash() != config->hash()) {
            qCWarning(DISMAN_BACKEND)
                << "Config control file not in sync. Was there a simultaneous hot-plug event?";
            return false;
        }
    } else {
        reset_filer(config);
    }

    return m_filer->write(config);
}

bool Filer_controller::load_lid_file(ConfigPtr& config)
{
    if (!lid_file_exists(config)) {
        qDebug(DISMAN_BACKEND) << "Loading open lid file failed: file does not exist.";
        return false;
    }
    if (!move_lid_file(config)) {
        qCWarning(DISMAN_BACKEND) << "Could not move open-lid file back to normal config file.";
    }
    reset_filer(config);
    return read(config);
}

bool Filer_controller::lid_file_exists(ConfigPtr const& config)
{
    auto const file_info = Filer(config, this, "open-lid").file_info();
    return QFile(file_info.filePath()).exists();
}

bool Filer_controller::move_lid_file(ConfigPtr const& config)
{
    assert(lid_file_exists(config));

    auto const file_info = Filer(config, this).file_info();
    QFile(file_info.filePath()).remove();

    auto const lid_file_info = Filer(config, this, "open-lid").file_info();
    return QFile::rename(lid_file_info.filePath(), file_info.filePath());
}

bool Filer_controller::save_lid_file(ConfigPtr const& config)
{
    return Filer(config, this, "open-lid").write(config);
}

void Filer_controller::reset_filer(ConfigPtr const& config)
{
    m_filer.reset(new Filer(config, this));
}

}
