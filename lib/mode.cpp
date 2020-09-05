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
#include "mode.h"

namespace Disman
{

class Q_DECL_HIDDEN Mode::Private
{
public:
    Private()
        : rate(0)
    {
    }

    Private(const Private& other)
        : id(other.id)
        , name(other.name)
        , size(other.size)
        , rate(other.rate)
    {
    }

    std::string id;
    std::string name;
    QSize size;
    double rate;
};

Mode::Mode()
    : d(new Private())
{
}

Mode::Mode(Mode::Private* dd)
    : d(dd)
{
}

Mode::~Mode()
{
    delete d;
}

ModePtr Mode::clone() const
{
    return ModePtr(new Mode(new Private(*d)));
}

std::string Mode::id() const
{
    return d->id;
}

void Mode::setId(std::string const& id)
{
    if (d->id == id) {
        return;
    }

    d->id = id;
}

std::string Mode::name() const
{
    return d->name;
}

void Mode::setName(std::string const& name)
{
    if (d->name == name) {
        return;
    }

    d->name = name;
}

QSize Mode::size() const
{
    return d->size;
}

void Mode::setSize(const QSize& size)
{
    if (d->size == size) {
        return;
    }

    d->size = size;
}

double Mode::refreshRate() const
{
    return d->rate;
}

void Mode::setRefreshRate(double refresh)
{
    if (qFuzzyCompare(d->rate, refresh)) {
        return;
    }

    d->rate = refresh;
}

}

QDebug operator<<(QDebug dbg, const Disman::ModePtr& mode)
{
    if (mode) {
        dbg << "Disman::Mode(Id:" << mode->id().c_str() << ", Size:" << mode->size() << "@"
            << mode->refreshRate() << ")";
    } else {
        dbg << "Disman::Mode(NULL)";
    }
    return dbg;
}
