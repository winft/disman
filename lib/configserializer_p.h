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

#include <QDBusArgument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariant>

#include "disman_export.h"
#include "output.h"
#include "types.h"

namespace Disman
{

namespace ConfigSerializer
{

DISMAN_EXPORT QJsonObject serialize_point(const QPointF& point);
DISMAN_EXPORT QJsonObject serialize_size(const QSize& size);
DISMAN_EXPORT QJsonObject serialize_sizef(const QSizeF& size);
template<typename T>
DISMAN_EXPORT QJsonArray serialize_list(const QList<T>& list)
{
    QJsonArray arr;
    Q_FOREACH (const T& t, list) {
        arr.append(t);
    }
    return arr;
}

DISMAN_EXPORT QJsonObject serialize_config(const Disman::ConfigPtr& config);
DISMAN_EXPORT QJsonObject serialize_output(const Disman::OutputPtr& output);
DISMAN_EXPORT QJsonObject serialize_mode(const Disman::ModePtr& mode);
DISMAN_EXPORT QJsonObject serialize_screen(const Disman::ScreenPtr& screen);

DISMAN_EXPORT QPointF deserialize_point(const QDBusArgument& map);
DISMAN_EXPORT QSize deserialize_size(const QDBusArgument& map);
DISMAN_EXPORT QSizeF deserialize_sizef(const QDBusArgument& map);
template<typename T>
DISMAN_EXPORT QList<T> deserialize_list(const QDBusArgument& arg)
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
DISMAN_EXPORT Disman::ConfigPtr deserialize_config(const QVariantMap& map);
DISMAN_EXPORT Disman::OutputPtr deserialize_output(const QDBusArgument& output);
DISMAN_EXPORT Disman::ModePtr deserialize_mode(const QDBusArgument& mode);
DISMAN_EXPORT Disman::ScreenPtr deserialize_screen(const QDBusArgument& screen);
DISMAN_EXPORT Disman::Output::Retention deserialize_retention(QVariant const& var);

}

}

#endif
