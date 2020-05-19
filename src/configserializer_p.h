/*
 * Copyright (C) 2014  Daniel Vratil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef CONFIGSERIALIZER_H
#define CONFIGSERIALIZER_H

#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QDBusArgument>

#include "types.h"
#include "disman_export.h"

namespace Disman
{

namespace ConfigSerializer
{

DISMAN_EXPORT QJsonObject serializePoint(const QPointF &point);
DISMAN_EXPORT QJsonObject serializeSize(const QSize &size);
DISMAN_EXPORT QJsonObject serializeSizeF(const QSizeF &size);
template<typename T>
DISMAN_EXPORT  QJsonArray serializeList(const QList<T> &list)
{
    QJsonArray arr;
    Q_FOREACH (const T &t, list) {
        arr.append(t);
    }
    return arr;
}

DISMAN_EXPORT QJsonObject serializeConfig(const Disman::ConfigPtr &config);
DISMAN_EXPORT QJsonObject serializeOutput(const Disman::OutputPtr &output);
DISMAN_EXPORT QJsonObject serializeMode(const Disman::ModePtr &mode);
DISMAN_EXPORT QJsonObject serializeScreen(const Disman::ScreenPtr &screen);

DISMAN_EXPORT QPointF deserializePoint(const QDBusArgument &map);
DISMAN_EXPORT QSize deserializeSize(const QDBusArgument &map);
DISMAN_EXPORT QSizeF deserializeSizeF(const QDBusArgument &map);
template<typename T>
DISMAN_EXPORT QList<T> deserializeList(const QDBusArgument &arg)
{
    QList<T> list;
    arg.beginArray();
    while (!arg.atEnd()) {
        QVariant v;
        arg >> v;
        list.append(v.value<T>());
    }
    arg.endArray();
    return list;
}
DISMAN_EXPORT Disman::ConfigPtr deserializeConfig(const QVariantMap &map);
DISMAN_EXPORT Disman::OutputPtr deserializeOutput(const QDBusArgument &output);
DISMAN_EXPORT Disman::ModePtr deserializeMode(const QDBusArgument &mode);
DISMAN_EXPORT Disman::ScreenPtr deserializeScreen(const QDBusArgument &screen);

}

}

#endif // CONFIGSERIALIZER_H
