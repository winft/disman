/*************************************************************************************
 *  Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *  Copyright (C) 2014 by Daniel Vrátil <dvratil@redhat.com>                         *
 *                                                                                   *
 *  This library is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU Lesser General Public                       *
 *  License as published by the Free Software Foundation; either                     *
 *  version 2.1 of the License, or (at your option) any later version.               *
 *                                                                                   *
 *  This library is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                *
 *  Lesser General Public License for more details.                                  *
 *                                                                                   *
 *  You should have received a copy of the GNU Lesser General Public                 *
 *  License along with this library; if not, write to the Free Software              *
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA       *
 *************************************************************************************/
#ifndef MODE_CONFIG_H
#define MODE_CONFIG_H

#include "disman_export.h"
#include "types.h"

#include <QDebug>
#include <QSize>
#include <string>

namespace Disman
{

class DISMAN_EXPORT Mode
{
public:
    Mode();
    ~Mode();

    ModePtr clone() const;

    std::string id() const;
    void set_id(std::string const& id);

    std::string name() const;
    void set_name(std::string const& name);

    QSize size() const;
    void set_size(const QSize& size);

    double refresh() const;
    void set_refresh(double refresh);

private:
    Q_DISABLE_COPY(Mode)

    class Private;
    Private* const d;

    Mode(Private* dd);
};

}

DISMAN_EXPORT QDebug operator<<(QDebug dbg, const Disman::ModePtr& mode);

#endif
