/*************************************************************************************
 *  Copyright 2014-2015 Sebastian Kügler <sebas@kde.org>                             *
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
#include "config_reader.h"

#include <QDebug>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

#include "edid.h"

using namespace Disman;

void WaylandConfigReader::outputsFromConfig(
    std::string const& configfile,
    Wrapland::Server::output_manager& manager,
    std::vector<std::unique_ptr<Wrapland::Server::output>>& outputs)
{
    qDebug() << "Loading server from" << configfile.c_str();
    QFile file(configfile.c_str());
    file.open(QIODevice::ReadOnly);

    QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll());
    QJsonObject json = jsonDoc.object();

    QJsonArray omap = json[QStringLiteral("outputs")].toArray();
    for (auto const& value : qAsConst(omap)) {
        auto const& output = value.toObject().toVariantMap();
        if (output[QStringLiteral("connected")].toBool()) {
            outputs.push_back(create_output(output, manager));
        }
    }
}

std::unique_ptr<Wrapland::Server::output>
WaylandConfigReader::create_output(QVariantMap const& outputConfig,
                                   Wrapland::Server::output_manager& manager)
{
    QByteArray data = QByteArray::fromBase64(outputConfig[QStringLiteral("edid")].toByteArray());
    Edid edid(data);
    Wrapland::Server::output_metadata output_meta;

    // qDebug() << "EDID Info: ";
    if (edid.isValid()) {
        // qDebug() << "\tDevice ID: " << edid.deviceId();
        // qDebug() << "\tName: " << edid.name();
        // qDebug() << "\tVendor: " << edid.vendor();
        // qDebug() << "\tSerial: " << edid.serial();
        // qDebug() << "\tEISA ID: " << edid.eisaId();
        // qDebug() << "\tHash: " << edid.hash();
        // qDebug() << "\tWidth (mm): " << edid.width();
        // qDebug() << "\tHeight (mm): " << edid.height();
        // qDebug() << "\tGamma: " << edid.gamma();
        // qDebug() << "\tRed: " << edid.red();
        // qDebug() << "\tGreen: " << edid.green();
        // qDebug() << "\tBlue: " << edid.blue();
        // qDebug() << "\tWhite: " << edid.white();
        output_meta.physical_size
            = {static_cast<int>(edid.width()) * 10, static_cast<int>(edid.height()) * 10};
        output_meta.make = edid.vendor();
        output_meta.model = edid.name();
        output_meta.serial_number = edid.serial();
    } else {
        output_meta.physical_size = sizeFromJson(outputConfig[QStringLiteral("physical_size")]);
        output_meta.make = outputConfig[QStringLiteral("manufacturer")].toString().toStdString();
        output_meta.model = outputConfig[QStringLiteral("model")].toString().toStdString();
    }

    auto uuid = QUuid::createUuid().toByteArray();
    auto _id = outputConfig[QStringLiteral("id")].toInt();
    if (_id) {
        uuid = QString::number(_id).toLocal8Bit();
    }
    output_meta.name = "out-" + uuid.toStdString();

    const QMap<int, Wrapland::Server::output_transform> transformMap = {
        {0, Wrapland::Server::output_transform::normal},
        {1, Wrapland::Server::output_transform::normal},
        {2, Wrapland::Server::output_transform::rotated_270},
        {3, Wrapland::Server::output_transform::rotated_180},
        {4, Wrapland::Server::output_transform::rotated_90},
    };

    auto output = std::make_unique<Wrapland::Server::output>(output_meta, manager);

    int currentModeId = outputConfig[QStringLiteral("currentModeId")].toInt();
    QVariantList preferredModes = outputConfig[QStringLiteral("preferredModes")].toList();
    auto const mode_list = outputConfig[QStringLiteral("modes")].toList();

    int mode_id = 0;
    Wrapland::Server::output_mode current_mode;
    bool has_current_mode{false};

    for (auto const& _mode : mode_list) {
        mode_id++;
        const QVariantMap& mode = _mode.toMap();
        Wrapland::Server::output_mode m0;

        m0.size = sizeFromJson(mode[QStringLiteral("size")]);

        auto refreshRateIt = mode.constFind(QStringLiteral("refreshRate"));
        if (refreshRateIt != mode.constEnd()) {
            // config has it in Hz
            m0.refresh_rate = qRound(refreshRateIt->toReal() * 1000);
        }

        if (preferredModes.contains(mode[QStringLiteral("id")])) {
            m0.preferred = true;
        }

        auto idIt = mode.constFind(QStringLiteral("id"));
        if (idIt != mode.constEnd()) {
            m0.id = idIt->toInt();
        } else {
            m0.id = mode_id;
        }

        output->add_mode(m0);

        if (!has_current_mode && currentModeId == m0.id) {
            has_current_mode = true;
            current_mode = m0;
        }
    }

    auto state = output->get_state();

    if (has_current_mode) {
        state.mode = current_mode;
    }

    state.transform = transformMap[outputConfig[QStringLiteral("rotation")].toInt()];
    state.geometry = {pointFromJson(outputConfig[QStringLiteral("pos")]), state.mode.size};
    state.enabled = outputConfig[QStringLiteral("enabled")].toBool();

    output->set_state(state);
    return output;
}

QSize WaylandConfigReader::sizeFromJson(const QVariant& data)
{
    QVariantMap map = data.toMap();

    QSize size;
    size.setWidth(map[QStringLiteral("width")].toInt());
    size.setHeight(map[QStringLiteral("height")].toInt());

    return size;
}

QPoint WaylandConfigReader::pointFromJson(const QVariant& data)
{
    QVariantMap map = data.toMap();

    QPoint point;
    point.setX(map[QStringLiteral("x")].toInt());
    point.setY(map[QStringLiteral("y")].toInt());

    return point;
}

QRect WaylandConfigReader::rectFromJson(const QVariant& data)
{
    QRect rect;
    rect.setSize(WaylandConfigReader::sizeFromJson(data));
    rect.setBottomLeft(WaylandConfigReader::pointFromJson(data));

    return rect;
}
