/*************************************************************************************
 *   Copyright (C) 2012 by Daniel Nicoletti <dantti12@gmail.com>                     *
 *             (C) 2012, 2013 by Daniel Vrátil <dvratil@redhat.com>                  *
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
#ifndef DISMAN_EDID_H
#define DISMAN_EDID_H

#include "disman_export.h"

#include <QQuaternion>
#include <QtGlobal>

#include <memory>
#include <string>

namespace Disman
{

class DISMAN_EXPORT Edid
{
public:
    explicit Edid(const QByteArray& data);
    Edid(Edid const& edid);
    ~Edid();

    bool isValid() const;

    std::string deviceId() const;
    std::string name() const;
    std::string vendor() const;
    std::string serial() const;
    std::string eisaId() const;
    std::string hash() const;
    std::string pnpId() const;

    uint width() const;
    uint height() const;

    double gamma() const;
    QQuaternion red() const;
    QQuaternion green() const;
    QQuaternion blue() const;
    QQuaternion white() const;

private:
    class Private;
    std::unique_ptr<Private> const d_ptr;
};

}

Q_DECLARE_METATYPE(Disman::Edid*)

#endif
