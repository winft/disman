/*************************************************************************************
 *  Copyright 2018   Daniel Vrátil <dvratil@kde.org>                                 *
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

#include "utils.h"

#include <QVector>

Disman::Output::Type Utils::guessOutputType(const QString& type, const QString& name)
{
    static const auto embedded = {QLatin1String("LVDS"),
                                  QLatin1String("IDP"),
                                  QLatin1String("EDP"),
                                  QLatin1String("LCD"),
                                  QLatin1String("DSI")};

    for (const QLatin1String& pre : embedded) {
        if (name.startsWith(pre, Qt::CaseInsensitive)) {
            return Disman::Output::Panel;
        }
    }

    if (type.contains(QLatin1String("VGA"))) {
        return Disman::Output::VGA;
    } else if (type.contains(QLatin1String("DVI"))) {
        return Disman::Output::DVI;
    } else if (type.contains(QLatin1String("DVI-I"))) {
        return Disman::Output::DVII;
    } else if (type.contains(QLatin1String("DVI-A"))) {
        return Disman::Output::DVIA;
    } else if (type.contains(QLatin1String("DVI-D"))) {
        return Disman::Output::DVID;
    } else if (type.contains(QLatin1String("HDMI"))) {
        return Disman::Output::HDMI;
    } else if (type.contains(QLatin1String("Panel"))) {
        return Disman::Output::Panel;
    } else if (type.contains(QLatin1String("TV-Composite"))) {
        return Disman::Output::TVComposite;
    } else if (type.contains(QLatin1String("TV-SVideo"))) {
        return Disman::Output::TVSVideo;
    } else if (type.contains(QLatin1String("TV-Component"))) {
        return Disman::Output::TVComponent;
    } else if (type.contains(QLatin1String("TV-SCART"))) {
        return Disman::Output::TVSCART;
    } else if (type.contains(QLatin1String("TV-C4"))) {
        return Disman::Output::TVC4;
    } else if (type.contains(QLatin1String("TV"))) {
        return Disman::Output::TV;
    } else if (type.contains(QLatin1String("DisplayPort"))
               || type.startsWith(QLatin1String("DP"))) {
        return Disman::Output::DisplayPort;
    } else if (type.contains(QLatin1String("unknown"))) {
        return Disman::Output::Unknown;
    } else {
        return Disman::Output::Unknown;
    }
}
