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
#include "screen.h"

using namespace Disman;

class Q_DECL_HIDDEN Screen::Private
{
public:
    Private()
        : id(0)
        , max_outputs_count(0)
    {
    }

    Private(const Private& other)
        : id(other.id)
        , max_outputs_count(other.max_outputs_count)
        , current_size(other.current_size)
        , min_size(other.min_size)
        , max_size(other.max_size)
    {
    }

    int id;
    int max_outputs_count;
    QSize current_size;
    QSize min_size;
    QSize max_size;
};

Screen::Screen()
    : QObject(nullptr)
    , d(new Private())
{
}

Screen::Screen(Screen::Private* dd)
    : QObject()
    , d(dd)
{
}

Screen::~Screen()
{
    delete d;
}

ScreenPtr Screen::clone() const
{
    return ScreenPtr(new Screen(new Private(*d)));
}

int Screen::id() const
{
    return d->id;
}

void Screen::set_id(int id)
{
    d->id = id;
}

QSize Screen::current_size() const
{
    return d->current_size;
}

void Screen::set_current_size(const QSize& current_size)
{
    if (d->current_size == current_size) {
        return;
    }

    d->current_size = current_size;

    Q_EMIT current_size_changed();
}

QSize Screen::max_size() const
{
    return d->max_size;
}

void Screen::set_max_size(const QSize& max_size)
{
    d->max_size = max_size;
}

QSize Screen::min_size() const
{
    return d->min_size;
}

void Screen::set_min_size(const QSize& min_size)
{
    d->min_size = min_size;
}

int Screen::max_outputs_count() const
{
    return d->max_outputs_count;
}

void Screen::set_max_outputs_count(int max_outputs_count)
{
    d->max_outputs_count = max_outputs_count;
}

void Screen::apply(const ScreenPtr& other)
{
    // Only set values that can change
    set_max_outputs_count(other->d->max_outputs_count);
    set_current_size(other->d->current_size);
}
