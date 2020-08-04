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
#pragma once

#include "types.h"

#include "disman_export.h"

#include <QObject>

#include <memory>

namespace Disman
{
class Filer;

/**
 * Side-channel controller for writing additional data to control files through the @ref Filer and
 * Output_filer class.
 */
class Filer_controller : public QObject
{
    Q_OBJECT
public:
    explicit Filer_controller(QObject* parent = nullptr);
    ~Filer_controller() override;

    void read(ConfigPtr& config);
    bool write(ConfigPtr const& config);

private:
    void reset_filer(ConfigPtr const& config);

    std::unique_ptr<Filer> m_filer;
};

}
