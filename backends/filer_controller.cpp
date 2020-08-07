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

#include "../logging.h"
#include "filer.h"

namespace Disman
{

Filer_controller::Filer_controller(QObject* parent)
    : QObject(parent)
{
}

Filer_controller::~Filer_controller() = default;

bool Filer_controller::read(ConfigPtr& config)
{
    if (m_filer) {
        if (m_filer->config()->connectedOutputsHash() != config->connectedOutputsHash()) {
            // Different output combination means different combination of filers.
            reset_filer(config);
        }
    } else {
        reset_filer(config);
    }

    return m_filer->get_values(config);
}

bool Filer_controller::write(ConfigPtr const& config)
{
    if (m_filer) {
        if (m_filer->config()->connectedOutputsHash() != config->connectedOutputsHash()) {
            qCWarning(DISMAN_BACKEND)
                << "Config control file not in sync. Was there a simultaneous hot-plug event?";
            return false;
        }
    } else {
        reset_filer(config);
    }

    return m_filer->write(config);
}

void Filer_controller::reset_filer(ConfigPtr const& config)
{
    m_filer.reset(new Filer(config, this));
}

}
