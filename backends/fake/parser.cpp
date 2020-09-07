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
#include "parser.h"
#include "fake.h"

#include "fake_logging.h"

#include "config.h"
#include "output.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QMetaProperty>

using namespace Disman;

ConfigPtr Parser::fromJson(const QByteArray& data)
{
    ConfigPtr config(new Config);

    const QJsonObject json = QJsonDocument::fromJson(data).object();

    ScreenPtr screen
        = Parser::screenFromJson(json[QStringLiteral("screen")].toObject().toVariantMap());
    config->setScreen(screen);

    const QVariantList outputs = json[QStringLiteral("outputs")].toArray().toVariantList();
    if (outputs.isEmpty()) {
        return config;
    }

    OutputMap outputList;
    OutputPtr primary;
    for (auto const& value : outputs) {
        bool is_primary = false;
        const OutputPtr output = Parser::outputFromJson(value.toMap(), is_primary);
        outputList.insert({output->id(), output});
        if (is_primary) {
            primary = output;
        }
    }

    config->set_outputs(outputList);
    config->set_primary_output(primary);
    return config;
}

ConfigPtr Parser::fromJson(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << file.errorString();
        qWarning() << "File: " << path;
        return ConfigPtr();
    }

    return Parser::fromJson(file.readAll());
}

ScreenPtr Parser::screenFromJson(const QVariantMap& data)
{
    ScreenPtr screen(new Screen);
    screen->set_id(data[QStringLiteral("id")].toInt());
    screen->set_min_size(Parser::sizeFromJson(data[QStringLiteral("minSize")].toMap()));
    screen->set_max_size(Parser::sizeFromJson(data[QStringLiteral("maxSize")].toMap()));
    screen->set_current_size(Parser::sizeFromJson(data[QStringLiteral("currentSize")].toMap()));
    screen->set_max_outputs_count(data[QStringLiteral("maxActiveOutputsCount")].toInt());

    return screen;
}

OutputPtr Parser::outputFromJson(QMap<QString, QVariant> map, bool& primary)
{
    OutputPtr output(new Output);
    output->set_id(map[QStringLiteral("id")].toInt());
    output->set_name(map[QStringLiteral("name")].toString().toStdString());
    output->set_description(map[QStringLiteral("description")].toString().toStdString());
    output->set_enabled(map[QStringLiteral("enabled")].toBool());
    output->set_rotation((Output::Rotation)map[QStringLiteral("rotation")].toInt());

    primary = map[QStringLiteral("primary")].toBool();

    std::vector<std::string> preferredModes;
    const QVariantList prefModes = map[QStringLiteral("preferredModes")].toList();
    for (auto const& mode : prefModes) {
        preferredModes.push_back(mode.toString().toStdString());
    }
    output->set_preferred_modes(preferredModes);

    ModeMap modelist;
    const QVariantList modes = map[QStringLiteral("modes")].toList();
    for (auto const& modeValue : modes) {
        const ModePtr mode = Parser::modeFromJson(modeValue);
        modelist.insert({mode->id(), mode});
    }
    output->set_modes(modelist);

    if (!map[QStringLiteral("currentModeId")].toString().isEmpty()) {
        output->set_mode(
            output->mode(map[QStringLiteral("currentModeId")].toString().toStdString()));
    }

    const QByteArray type = map[QStringLiteral("type")].toByteArray().toUpper();
    if (type.contains("LVDS") || type.contains("EDP") || type.contains("IDP")
        || type.contains("7")) {
        output->setType(Output::Panel);
    } else if (type.contains("VGA")) {
        output->setType(Output::VGA);
    } else if (type.contains("DVI")) {
        output->setType(Output::DVI);
    } else if (type.contains("DVI-I")) {
        output->setType(Output::DVII);
    } else if (type.contains("DVI-A")) {
        output->setType(Output::DVIA);
    } else if (type.contains("DVI-D")) {
        output->setType(Output::DVID);
    } else if (type.contains("HDMI") || type.contains("6")) {
        output->setType(Output::HDMI);
    } else if (type.contains("Panel")) {
        output->setType(Output::Panel);
    } else if (type.contains("TV")) {
        output->setType(Output::TV);
    } else if (type.contains("TV-Composite")) {
        output->setType(Output::TVComposite);
    } else if (type.contains("TV-SVideo")) {
        output->setType(Output::TVSVideo);
    } else if (type.contains("TV-Component")) {
        output->setType(Output::TVComponent);
    } else if (type.contains("TV-SCART")) {
        output->setType(Output::TVSCART);
    } else if (type.contains("TV-C4")) {
        output->setType(Output::TVC4);
    } else if (type.contains("DisplayPort") || type.contains("14")) {
        output->setType(Output::DisplayPort);
    } else if (type.contains("Unknown")) {
        output->setType(Output::Unknown);
    } else {
        qCWarning(DISMAN_FAKE) << "Output Type not translated:" << type;
    }

    if (map.contains(QStringLiteral("pos"))) {
        output->set_position(Parser::pointFromJson(map[QStringLiteral("pos")].toMap()));
    }

    auto scale = QStringLiteral("scale");
    if (map.contains(scale)) {
        qDebug() << "Scale found:" << map[scale].toReal();
        output->set_scale(map[scale].toReal());
    }

    return output;
}

ModePtr Parser::modeFromJson(const QVariant& data)
{
    const QVariantMap map = data.toMap();
    ModePtr mode(new Mode);

    mode->set_id(map[QStringLiteral("id")].toString().toStdString());
    mode->set_name(map[QStringLiteral("name")].toString().toStdString());
    mode->set_refresh(map[QStringLiteral("refresh")].toDouble());
    mode->set_size(Parser::sizeFromJson(map[QStringLiteral("size")].toMap()));

    return mode;
}

QSize Parser::sizeFromJson(const QVariant& data)
{
    const QVariantMap map = data.toMap();

    QSize size;
    size.setWidth(map[QStringLiteral("width")].toInt());
    size.setHeight(map[QStringLiteral("height")].toInt());

    return size;
}

QPoint Parser::pointFromJson(const QVariant& data)
{
    const QVariantMap map = data.toMap();

    QPoint point;
    point.setX(map[QStringLiteral("x")].toInt());
    point.setY(map[QStringLiteral("y")].toInt());

    return point;
}

QRect Parser::rectFromJson(const QVariant& data)
{
    QRect rect;
    rect.setSize(Parser::sizeFromJson(data));
    rect.setBottomLeft(Parser::pointFromJson(data));

    return rect;
}

bool Parser::validate(const QByteArray& data)
{
    Q_UNUSED(data);
    return true;
}

bool Parser::validate(const QString& data)
{
    Q_UNUSED(data);
    return true;
}
