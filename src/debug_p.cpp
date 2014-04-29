/*************************************************************************************
 *  Copyright (C) 2014 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
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

#include "debug_p.h"
#include <QString>
#include <QCoreApplication>

Q_LOGGING_CATEGORY(KSCREEN, "kscreen")
Q_LOGGING_CATEGORY(KSCREEN_EDID, "kscreen.edid")

static void enableAllDebug()
{
    QLoggingCategory::setFilterRules(QStringLiteral("kscreen.debug = true"));
    QLoggingCategory::setFilterRules(QStringLiteral("kscreen.edid.debug = true"));
}

Q_COREAPP_STARTUP_FUNCTION(enableAllDebug)