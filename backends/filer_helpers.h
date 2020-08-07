/*************************************************************************
Copyright © 2020 Roman Gilg <subdiff@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
**************************************************************************/
#pragma once

#include "../logging.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QVariant>
#include <QVariantMap>

namespace Disman::Filer_helpers
{

static void read_file(QFileInfo const& file_info, QVariantMap& info)
{
    QFile file(file_info.filePath());
    if (!file.exists()) {
        return;
    }

    if (file.open(QIODevice::ReadOnly)) {
        // This might not be reached, bus this is ok. The control file will
        // eventually be created on first write later on.
        QJsonDocument parser;
        info = parser.fromJson(file.readAll()).toVariant().toMap();
    } else {
        qCWarning(DISMAN_BACKEND) << "Failed to open config control file for reading."
                                  << file.errorString();
    }
}

static QFileInfo file_info(std::string const& dir_path, QString const& hash)
{
    return QFileInfo(QDir(QString::fromStdString(dir_path)), hash % QStringLiteral(".json"));
}

static bool write_file(QVariantMap const& map, QFileInfo const& file_info)
{
    if (map.isEmpty()) {
        // Nothing to write. Default control. Remove file if it exists.
        QFile::remove(file_info.filePath());
        return true;
    }
    if (!QDir().mkpath(file_info.path())) {
        // TODO: error message
        return false;
    }

    // write updated data to file
    QFile file(file_info.filePath());
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(DISMAN_BACKEND) << "Failed to open config control file for writing."
                                  << file.errorString();
        return false;
    }
    file.write(QJsonDocument::fromVariant(map).toJson());
    qCDebug(DISMAN_BACKEND) << "Control saved to:" << file.fileName();
    return true;
}

template<typename T>
static T from_variant(QVariant const& var, T default_value = T())
{
    if (var.canConvert<double>()) {
        return var.toDouble();
    } else if (var.canConvert<int>()) {
        return var.toInt();
    } else if (var.canConvert<bool>()) {
        return var.toBool();
    }
    return default_value;
}

template<>
QString from_variant(QVariant const& var, QString default_value)
{
    if (var.canConvert<QString>()) {
        return var.toString();
    }
    return default_value;
}

}
