/*************************************************************************************
 *  Copyright 2012, 2013  Daniel Vrátil <dvratil@redhat.com>                         *
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
#pragma once

#include <QObject>
#include <QSize>
#include <QVariant>

#include "types.h"
#include "xcbwrapper.h"

class XRandROutput;
namespace Disman
{
class Output;
class Mode;
}

class XRandRMode : public QObject
{
    Q_OBJECT

public:
    using Map = std::map<xcb_randr_mode_t, XRandRMode*>;

    explicit XRandRMode(const xcb_randr_mode_info_t& modeInfo, XRandROutput* output);
    ~XRandRMode() override;

    Disman::ModePtr toDismanMode();

    xcb_randr_mode_t id() const;
    QSize size() const;
    float refreshRate() const;
    QString name() const;
    bool doubleScan() const;

private:
    xcb_randr_mode_t m_id;
    QString m_name;
    QSize m_size;
    float m_refreshRate;
    bool m_doubleScan{false};
};

Q_DECLARE_METATYPE(XRandRMode::Map)
