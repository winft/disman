/*************************************************************************************
 *  Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
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
#ifndef SCREEN_CONFIG_H
#define SCREEN_CONFIG_H

#include "disman_export.h"
#include "types.h"

#include <QObject>
#include <QSize>

namespace Disman
{

class DISMAN_EXPORT Screen : public QObject
{
    Q_OBJECT

public:
    Q_PROPERTY(int id READ id CONSTANT)
    Q_PROPERTY(
        QSize current_size READ current_size WRITE set_current_size NOTIFY current_size_changed)
    Q_PROPERTY(QSize min_size READ min_size CONSTANT)
    Q_PROPERTY(QSize max_size READ max_size CONSTANT)
    Q_PROPERTY(int max_outputs_count READ max_outputs_count CONSTANT)

    Screen();
    ~Screen() override;

    ScreenPtr clone() const;

    /**
     * The id of this screen.
     * @return id of this screen
     */
    int id() const;
    /**
     * The identifier of this screen.
     * @param id id of the screen
     */
    void set_id(int id);

    /**
     * The current screen size in pixels.
     * @return Screen size in pixels
     */
    QSize current_size() const;
    /**
     * Set the current screen size in pixels.
     * @param current_size Screen size in pixels
     */
    void set_current_size(const QSize& current_size);

    /**
     * The minimum screen size in pixels.
     * @return Minimum screen size in pixels
     */
    QSize min_size() const;
    /**
     * Set the minimum screen size in pixels.
     * @param min_size Minimum screen size in pixels
     */
    void set_min_size(const QSize& min_size);

    /**
     * The maximum screen size in pixels.
     * @return Maximum screen size in pixels
     */
    QSize max_size() const;
    /**
     * Set the maximum screen size in pixels.
     * @param max_size Maximum screen size in pixels
     */
    void set_max_size(const QSize& max_size);

    int max_outputs_count() const;
    void set_max_outputs_count(int max_outputs_count);

    void apply(const ScreenPtr& other);

Q_SIGNALS:
    void current_size_changed();

private:
    Q_DISABLE_COPY(Screen)

    class Private;
    Private* const d;

    Screen(Private* dd);
};

}

#endif
