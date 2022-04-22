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
#include "configserializer_p.h"

#include "config.h"
#include "disman_debug.h"
#include "mode.h"
#include "screen.h"

#include <QDBusArgument>
#include <QFile>
#include <QJsonDocument>
#include <QRect>

#include <string>
#include <vector>

using namespace Disman;

QJsonObject ConfigSerializer::serialize_point(const QPointF& point)
{
    QJsonObject obj;
    obj[QLatin1String("x")] = point.x();
    obj[QLatin1String("y")] = point.y();
    return obj;
}

QJsonObject ConfigSerializer::serialize_size(const QSize& size)
{
    QJsonObject obj;
    obj[QLatin1String("width")] = size.width();
    obj[QLatin1String("height")] = size.height();
    return obj;
}

QJsonObject ConfigSerializer::serialize_sizef(const QSizeF& size)
{
    QJsonObject obj;
    obj[QLatin1String("width")] = size.width();
    obj[QLatin1String("height")] = size.height();
    return obj;
}

QJsonObject ConfigSerializer::serialize_config(const ConfigPtr& config)
{
    QJsonObject obj;

    if (!config) {
        return obj;
    }

    obj[QLatin1String("cause")] = static_cast<int>(config->cause());
    obj[QLatin1String("features")] = static_cast<int>(config->supported_features());
    if (auto primary = config->primary_output()) {
        obj[QLatin1String("primary-output")] = primary->id();
    }

    QJsonArray outputs;
    for (auto const& [key, output] : config->outputs()) {
        outputs.append(serialize_output(output));
    }
    obj[QLatin1String("outputs")] = outputs;
    if (config->screen()) {
        obj[QLatin1String("screen")] = serialize_screen(config->screen());
    }

    obj[QLatin1String("tablet_mode_available")] = config->tablet_mode_available();
    obj[QLatin1String("tablet_mode_engaged")] = config->tablet_mode_engaged();

    return obj;
}

QJsonObject ConfigSerializer::serialize_output(const OutputPtr& output)
{
    QJsonObject obj;

    obj[QLatin1String("id")] = output->id();
    obj[QLatin1String("name")] = QString::fromStdString(output->name());
    obj[QLatin1String("description")] = QString::fromStdString(output->description());
    obj[QLatin1String("hash")] = QString::fromStdString(output->hash());
    obj[QLatin1String("type")] = static_cast<int>(output->type());
    obj[QLatin1String("position")] = serialize_point(output->position());
    obj[QLatin1String("scale")] = output->scale();
    obj[QLatin1String("rotation")] = static_cast<int>(output->rotation());

    auto const mode = output->auto_mode();
    assert(mode);
    obj[QLatin1String("resolution")] = serialize_size(mode->size());
    obj[QLatin1String("refresh")] = mode->refresh();

    QStringList mode_q_strings;
    for (auto const& mode_string : output->preferred_modes()) {
        mode_q_strings.push_back(QString::fromStdString(mode_string));
    }
    obj[QLatin1String("preferred_modes")] = serialize_list(mode_q_strings);

    obj[QLatin1String("follow_preferred_mode")] = output->follow_preferred_mode();
    obj[QLatin1String("enabled")] = output->enabled();
    obj[QLatin1String("physical_size")] = serialize_size(output->physical_size());
    obj[QLatin1String("replication_source")] = output->replication_source();
    obj[QLatin1String("auto_rotate")] = output->auto_rotate();
    obj[QLatin1String("auto_rotate_only_in_tablet_mode")]
        = output->auto_rotate_only_in_tablet_mode();
    obj[QLatin1String("auto_resolution")] = output->auto_resolution();
    obj[QLatin1String("auto_refresh_rate")] = output->auto_refresh_rate();
    obj[QLatin1String("retention")] = static_cast<int>(output->retention());

    QJsonArray modes;
    for (auto const& [key, mode] : output->modes()) {
        modes.append(serialize_mode(mode));
    }
    obj[QLatin1String("modes")] = modes;

    auto data = output->global_data();
    if (data.valid) {
        obj[QLatin1String("global")] = true;

        obj[QLatin1String("global.resolution")] = serialize_size(data.resolution);
        obj[QLatin1String("global.refresh")] = data.refresh;

        obj[QLatin1String("global.rotation")] = data.rotation;
        obj[QLatin1String("global.scale")] = data.scale;

        obj[QLatin1String("global.auto_resolution")] = data.auto_resolution;
        obj[QLatin1String("global.auto_refresh_rate")] = data.auto_refresh_rate;

        obj[QLatin1String("global.auto_rotate")] = data.auto_rotate;
        obj[QLatin1String("global.auto_rotate_only_in_tablet_mode")]
            = data.auto_rotate_only_in_tablet_mode;
    }

    return obj;
}

QJsonObject ConfigSerializer::serialize_mode(const ModePtr& mode)
{
    QJsonObject obj;

    obj[QLatin1String("id")] = QString::fromStdString(mode->id());
    obj[QLatin1String("name")] = QString::fromStdString(mode->name());
    obj[QLatin1String("size")] = serialize_size(mode->size());
    obj[QLatin1String("refresh")] = mode->refresh();

    return obj;
}

QJsonObject ConfigSerializer::serialize_screen(const ScreenPtr& screen)
{
    QJsonObject obj;

    obj[QLatin1String("id")] = screen->id();
    obj[QLatin1String("current_size")] = serialize_size(screen->current_size());
    obj[QLatin1String("max_size")] = serialize_size(screen->max_size());
    obj[QLatin1String("min_size")] = serialize_size(screen->min_size());
    obj[QLatin1String("max_outputs_count")] = screen->max_outputs_count();

    return obj;
}

QPointF ConfigSerializer::deserialize_point(const QDBusArgument& arg)
{
    double x = 0;
    double y = 0;
    arg.beginMap();

    while (!arg.atEnd()) {
        QString key;
        QVariant value;
        arg.beginMapEntry();
        arg >> key >> value;
        if (key == QLatin1Char('x')) {
            x = value.toDouble();
        } else if (key == QLatin1Char('y')) {
            y = value.toDouble();
        } else {
            qCWarning(DISMAN) << "Invalid key in Point map: " << key;
            return QPointF();
        }
        arg.endMapEntry();
    }

    arg.endMap();
    return QPointF(x, y);
}

Output::Retention ConfigSerializer::deserialize_retention(QVariant const& var)
{
    if (var.canConvert<int>()) {
        auto val = var.toInt();
        if (val == static_cast<int>(Output::Retention::Global)) {
            return Output::Retention::Global;
        }
        if (val == static_cast<int>(Output::Retention::Individual)) {
            return Output::Retention::Individual;
        }
    }
    return Output::Retention::Undefined;
}

QSize ConfigSerializer::deserialize_size(const QDBusArgument& arg)
{
    int w = 0, h = 0;
    arg.beginMap();
    while (!arg.atEnd()) {
        QString key;
        QVariant value;
        arg.beginMapEntry();
        arg >> key >> value;
        if (key == QLatin1String("width")) {
            w = value.toInt();
        } else if (key == QLatin1String("height")) {
            h = value.toInt();
        } else {
            qCWarning(DISMAN) << "Invalid key in size struct: " << key;
            return QSize();
        }
        arg.endMapEntry();
    }
    arg.endMap();

    return QSize(w, h);
}

QSizeF ConfigSerializer::deserialize_sizef(const QDBusArgument& arg)
{
    double w = 0;
    double h = 0;
    arg.beginMap();
    while (!arg.atEnd()) {
        QString key;
        QVariant value;
        arg.beginMapEntry();
        arg >> key >> value;
        if (key == QLatin1String("width")) {
            w = value.toDouble();
        } else if (key == QLatin1String("height")) {
            h = value.toDouble();
        } else {
            qCWarning(DISMAN) << "Invalid key in size struct: " << key;
            return QSize();
        }
        arg.endMapEntry();
    }
    arg.endMap();

    return QSizeF(w, h);
}

ConfigPtr ConfigSerializer::deserialize_config(const QVariantMap& map)
{
    auto cause = static_cast<Config::Cause>(
        map.value(QStringLiteral("cause"), static_cast<int>(Config::Cause::unknown)).toInt());
    switch (cause) {
    case Config::Cause::unknown:
    case Config::Cause::generated:
    case Config::Cause::file:
    case Config::Cause::interactive:
        break;
    default:
        qCWarning(DISMAN) << "Deserialized config without valid cause value.";
        cause = Config::Cause::unknown;
    }

    ConfigPtr config(new Config(cause));

    if (map.contains(QLatin1String("features"))) {
        config->set_supported_features(
            static_cast<Config::Features>(map[QStringLiteral("features")].toInt()));
    }

    if (map.contains(QLatin1String("tablet_mode_available"))) {
        config->set_tablet_mode_available(map[QStringLiteral("tablet_mode_available")].toBool());
    }
    if (map.contains(QLatin1String("tablet_mode_engaged"))) {
        config->set_tablet_mode_engaged(map[QStringLiteral("tablet_mode_engaged")].toBool());
    }

    if (map.contains(QLatin1String("outputs"))) {
        const QDBusArgument& outputsArg = map[QStringLiteral("outputs")].value<QDBusArgument>();
        outputsArg.beginArray();
        OutputMap outputs;
        while (!outputsArg.atEnd()) {
            QVariant value;
            outputsArg >> value;
            const Disman::OutputPtr output = deserialize_output(value.value<QDBusArgument>());
            if (!output) {
                return ConfigPtr();
            }
            outputs.insert({output->id(), output});
        }
        outputsArg.endArray();
        config->set_outputs(outputs);
    }

    if (map.contains(QLatin1String("primary-output"))) {
        auto const id = map[QStringLiteral("primary-output")].toInt();
        auto output = config->output(id);
        if (!output) {
            return ConfigPtr();
        }
        config->set_primary_output(output);
    }

    if (map.contains(QLatin1String("screen"))) {
        const QDBusArgument& screenArg = map[QStringLiteral("screen")].value<QDBusArgument>();
        const Disman::ScreenPtr screen = deserialize_screen(screenArg);
        if (!screen) {
            return ConfigPtr();
        }
        config->setScreen(screen);
    }

    return config;
}

OutputPtr ConfigSerializer::deserialize_output(const QDBusArgument& arg)
{
    OutputPtr output(new Output);
    Output::GlobalData global_data;

    arg.beginMap();
    while (!arg.atEnd()) {
        QString key;
        QVariant value;
        arg.beginMapEntry();
        arg >> key >> value;
        if (key == QLatin1String("id")) {
            output->set_id(value.toInt());
        } else if (key == QLatin1String("name")) {
            output->set_name(value.toString().toStdString());
        } else if (key == QLatin1String("description")) {
            output->set_description(value.toString().toStdString());
        } else if (key == QLatin1String("hash")) {
            output->set_hash_raw(value.toString().toStdString());
        } else if (key == QLatin1String("type")) {
            output->setType(static_cast<Output::Type>(value.toInt()));
        } else if (key == QLatin1String("position")) {
            output->set_position(deserialize_point(value.value<QDBusArgument>()));
        } else if (key == QLatin1String("scale")) {
            output->set_scale(value.toDouble());
        } else if (key == QLatin1String("rotation")) {
            output->set_rotation(static_cast<Output::Rotation>(value.toInt()));
        } else if (key == QLatin1String("resolution")) {
            output->set_resolution(deserialize_size(value.value<QDBusArgument>()));
        } else if (key == QLatin1String("refresh")) {
            output->set_refresh_rate(value.toInt());
        } else if (key == QLatin1String("auto_rotate")) {
            output->set_auto_rotate(value.toBool());
        } else if (key == QLatin1String("auto_rotate_only_in_tablet_mode")) {
            output->set_auto_rotate_only_in_tablet_mode(value.toBool());
        } else if (key == QLatin1String("auto_resolution")) {
            output->set_auto_resolution(value.toBool());
        } else if (key == QLatin1String("auto_refresh_rate")) {
            output->set_auto_refresh_rate(value.toBool());
        }

        else if (key == QLatin1String("global")) {
            global_data.valid = true;
        } else if (key == QLatin1String("global.resolution")) {
            global_data.resolution = deserialize_size(value.value<QDBusArgument>());
        } else if (key == QLatin1String("global.refresh")) {
            global_data.refresh = value.toInt();
        } else if (key == QLatin1String("global.rotation")) {
            global_data.rotation = static_cast<Output::Rotation>(value.toInt());
        } else if (key == QLatin1String("global.scale")) {
            global_data.scale = value.toDouble();
        } else if (key == QLatin1String("global.auto_resolution")) {
            global_data.auto_resolution = value.toBool();
        } else if (key == QLatin1String("global.auto_refresh_rate")) {
            global_data.auto_refresh_rate = value.toBool();
        } else if (key == QLatin1String("global.auto_rotate")) {
            global_data.auto_rotate = value.toBool();
        } else if (key == QLatin1String("global.auto_rotate_only_in_tablet_mode")) {
            global_data.auto_rotate_only_in_tablet_mode = value.toBool();
        }

        else if (key == QLatin1String("preferred_modes")) {
            auto q_strings = deserialize_list<QString>(value.value<QDBusArgument>());
            std::vector<std::string> strings;
            for (auto const& qs : q_strings) {
                strings.push_back(qs.toStdString());
            }
            output->set_preferred_modes(strings);

        } else if (key == QLatin1String("follow_preferred_mode")) {
            output->set_follow_preferred_mode(value.toBool());
        } else if (key == QLatin1String("enabled")) {
            output->set_enabled(value.toBool());
        } else if (key == QLatin1String("replication_source")) {
            output->set_replication_source(value.toInt());
        } else if (key == QLatin1String("physical_size")) {
            output->set_physical_size(deserialize_size(value.value<QDBusArgument>()));
        } else if (key == QLatin1String("retention")) {
            output->set_retention(deserialize_retention(value));
        } else if (key == QLatin1String("modes")) {
            const QDBusArgument arg = value.value<QDBusArgument>();
            ModeMap modes;
            arg.beginArray();
            while (!arg.atEnd()) {
                QVariant value;
                arg >> value;
                const Disman::ModePtr mode = deserialize_mode(value.value<QDBusArgument>());
                if (!mode) {
                    return OutputPtr();
                }
                modes.insert({mode->id(), mode});
            }
            arg.endArray();
            output->set_modes(modes);
        } else {
            qCWarning(DISMAN) << "Invalid key in Output map: " << key;
            return OutputPtr();
        }
        arg.endMapEntry();
    }
    arg.endMap();

    if (global_data.valid) {
        output->set_global_data(global_data);
    }
    return output;
}

ModePtr ConfigSerializer::deserialize_mode(const QDBusArgument& arg)
{
    ModePtr mode(new Mode);

    arg.beginMap();
    while (!arg.atEnd()) {
        QString key;
        QVariant value;
        arg.beginMapEntry();
        arg >> key >> value;

        if (key == QLatin1String("id")) {
            mode->set_id(value.toString().toStdString());
        } else if (key == QLatin1String("name")) {
            mode->set_name(value.toString().toStdString());
        } else if (key == QLatin1String("size")) {
            mode->set_size(deserialize_size(value.value<QDBusArgument>()));
        } else if (key == QLatin1String("refresh")) {
            mode->set_refresh(value.toFloat());
        } else {
            qCWarning(DISMAN) << "Invalid key in Mode map: " << key;
            return ModePtr();
        }
        arg.endMapEntry();
    }
    arg.endMap();
    return mode;
}

ScreenPtr ConfigSerializer::deserialize_screen(const QDBusArgument& arg)
{
    ScreenPtr screen(new Screen);

    arg.beginMap();
    QString key;
    QVariant value;
    while (!arg.atEnd()) {
        arg.beginMapEntry();
        arg >> key >> value;
        if (key == QLatin1String("id")) {
            screen->set_id(value.toInt());
        } else if (key == QLatin1String("max_outputs_count")) {
            screen->set_max_outputs_count(value.toInt());
        } else if (key == QLatin1String("current_size")) {
            screen->set_current_size(deserialize_size(value.value<QDBusArgument>()));
        } else if (key == QLatin1String("max_size")) {
            screen->set_max_size(deserialize_size(value.value<QDBusArgument>()));
        } else if (key == QLatin1String("min_size")) {
            screen->set_min_size(deserialize_size(value.value<QDBusArgument>()));
        } else {
            qCWarning(DISMAN) << "Invalid key in Screen map:" << key;
            return ScreenPtr();
        }
        arg.endMapEntry();
    }
    arg.endMap();
    return screen;
}
