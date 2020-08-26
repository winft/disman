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
#include "edid.h"
#include "mode.h"
#include "screen.h"

#include <QDBusArgument>
#include <QFile>
#include <QJsonDocument>
#include <QRect>

using namespace Disman;

QJsonObject ConfigSerializer::serializePoint(const QPointF& point)
{
    QJsonObject obj;
    obj[QLatin1String("x")] = point.x();
    obj[QLatin1String("y")] = point.y();
    return obj;
}

QJsonObject ConfigSerializer::serializeSize(const QSize& size)
{
    QJsonObject obj;
    obj[QLatin1String("width")] = size.width();
    obj[QLatin1String("height")] = size.height();
    return obj;
}

QJsonObject ConfigSerializer::serializeSizeF(const QSizeF& size)
{
    QJsonObject obj;
    obj[QLatin1String("width")] = size.width();
    obj[QLatin1String("height")] = size.height();
    return obj;
}

QJsonObject ConfigSerializer::serializeConfig(const ConfigPtr& config)
{
    QJsonObject obj;

    if (!config) {
        return obj;
    }

    obj[QLatin1String("origin")] = static_cast<int>(config->origin());
    obj[QLatin1String("features")] = static_cast<int>(config->supportedFeatures());

    QJsonArray outputs;
    Q_FOREACH (const OutputPtr& output, config->outputs()) {
        outputs.append(serializeOutput(output));
    }
    obj[QLatin1String("outputs")] = outputs;
    if (config->screen()) {
        obj[QLatin1String("screen")] = serializeScreen(config->screen());
    }

    obj[QLatin1String("tabletModeAvailable")] = config->tabletModeAvailable();
    obj[QLatin1String("tabletModeEngaged")] = config->tabletModeEngaged();

    return obj;
}

QJsonObject ConfigSerializer::serializeOutput(const OutputPtr& output)
{
    QJsonObject obj;

    obj[QLatin1String("id")] = output->id();
    obj[QLatin1String("name")] = QString::fromStdString(output->name());
    obj[QLatin1String("type")] = static_cast<int>(output->type());
    obj[QLatin1String("icon")] = output->icon();
    obj[QLatin1String("position")] = serializePoint(output->position());
    obj[QLatin1String("scale")] = output->scale();
    obj[QLatin1String("rotation")] = static_cast<int>(output->rotation());
    if (auto const mode = output->commanded_mode()) {
        obj[QLatin1String("resolution")] = serializeSize(mode->size());
        obj[QLatin1String("refresh_rate")] = mode->refreshRate();
    }
    obj[QLatin1String("preferredModes")] = serializeList(output->preferredModes());
    obj[QLatin1String("followPreferredMode")] = output->followPreferredMode();
    obj[QLatin1String("enabled")] = output->isEnabled();
    obj[QLatin1String("primary")] = output->isPrimary();
    // obj[QLatin1String("edid")] = output->edid()->raw();
    obj[QLatin1String("sizeMM")] = serializeSize(output->sizeMm());
    obj[QLatin1String("replicationSource")] = output->replicationSource();
    obj[QLatin1String("auto_rotate")] = output->auto_rotate();
    obj[QLatin1String("auto_rotate_only_in_tablet_mode")]
        = output->auto_rotate_only_in_tablet_mode();
    obj[QLatin1String("auto_resolution")] = output->auto_resolution();
    obj[QLatin1String("auto_refresh_rate")] = output->auto_refresh_rate();
    obj[QLatin1String("retention")] = static_cast<int>(output->retention());

    QJsonArray modes;
    Q_FOREACH (const ModePtr& mode, output->modes()) {
        modes.append(serializeMode(mode));
    }
    obj[QLatin1String("modes")] = modes;

    return obj;
}

QJsonObject ConfigSerializer::serializeMode(const ModePtr& mode)
{
    QJsonObject obj;

    obj[QLatin1String("id")] = mode->id();
    obj[QLatin1String("name")] = mode->name();
    obj[QLatin1String("size")] = serializeSize(mode->size());
    obj[QLatin1String("refreshRate")] = mode->refreshRate();

    return obj;
}

QJsonObject ConfigSerializer::serializeScreen(const ScreenPtr& screen)
{
    QJsonObject obj;

    obj[QLatin1String("id")] = screen->id();
    obj[QLatin1String("currentSize")] = serializeSize(screen->currentSize());
    obj[QLatin1String("maxSize")] = serializeSize(screen->maxSize());
    obj[QLatin1String("minSize")] = serializeSize(screen->minSize());
    obj[QLatin1String("maxActiveOutputsCount")] = screen->maxActiveOutputsCount();

    return obj;
}

QPointF ConfigSerializer::deserializePoint(const QDBusArgument& arg)
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

QSize ConfigSerializer::deserializeSize(const QDBusArgument& arg)
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

QSizeF ConfigSerializer::deserializeSizeF(const QDBusArgument& arg)
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

ConfigPtr ConfigSerializer::deserializeConfig(const QVariantMap& map)
{
    auto origin = static_cast<Config::Origin>(
        map.value(QStringLiteral("origin"), static_cast<int>(Config::Origin::unknown)).toInt());
    switch (origin) {
    case Config::Origin::unknown:
    case Config::Origin::generated:
    case Config::Origin::file:
        break;
    default:
        qCWarning(DISMAN) << "Deserialized config without valid origin value.";
        origin = Config::Origin::unknown;
    }

    ConfigPtr config(new Config(origin));

    if (map.contains(QLatin1String("features"))) {
        config->setSupportedFeatures(
            static_cast<Config::Features>(map[QStringLiteral("features")].toInt()));
    }

    if (map.contains(QLatin1String("tabletModeAvailable"))) {
        config->setTabletModeAvailable(map[QStringLiteral("tabletModeAvailable")].toBool());
    }
    if (map.contains(QLatin1String("tabletModeEngaged"))) {
        config->setTabletModeEngaged(map[QStringLiteral("tabletModeEngaged")].toBool());
    }

    if (map.contains(QLatin1String("outputs"))) {
        const QDBusArgument& outputsArg = map[QStringLiteral("outputs")].value<QDBusArgument>();
        outputsArg.beginArray();
        OutputList outputs;
        while (!outputsArg.atEnd()) {
            QVariant value;
            outputsArg >> value;
            const Disman::OutputPtr output = deserializeOutput(value.value<QDBusArgument>());
            if (!output) {
                return ConfigPtr();
            }
            outputs.insert(output->id(), output);
        }
        outputsArg.endArray();
        config->setOutputs(outputs);
    }

    if (map.contains(QLatin1String("screen"))) {
        const QDBusArgument& screenArg = map[QStringLiteral("screen")].value<QDBusArgument>();
        const Disman::ScreenPtr screen = deserializeScreen(screenArg);
        if (!screen) {
            return ConfigPtr();
        }
        config->setScreen(screen);
    }

    return config;
}

OutputPtr ConfigSerializer::deserializeOutput(const QDBusArgument& arg)
{
    OutputPtr output(new Output);

    arg.beginMap();
    while (!arg.atEnd()) {
        QString key;
        QVariant value;
        arg.beginMapEntry();
        arg >> key >> value;
        if (key == QLatin1String("id")) {
            output->setId(value.toInt());
        } else if (key == QLatin1String("name")) {
            output->set_name(value.toString().toStdString());
        } else if (key == QLatin1String("type")) {
            output->setType(static_cast<Output::Type>(value.toInt()));
        } else if (key == QLatin1String("icon")) {
            output->setIcon(value.toString());
        } else if (key == QLatin1String("position")) {
            output->setPosition(deserializePoint(value.value<QDBusArgument>()));
        } else if (key == QLatin1String("scale")) {
            output->setScale(value.toDouble());
        } else if (key == QLatin1String("rotation")) {
            output->setRotation(static_cast<Output::Rotation>(value.toInt()));
        } else if (key == QLatin1String("resolution")) {
            output->set_resolution(value.toSize());
        } else if (key == QLatin1String("refresh_rate")) {
            output->set_refresh_rate(value.toDouble());
        } else if (key == QLatin1String("auto_rotate")) {
            output->set_auto_rotate(value.toBool());
        } else if (key == QLatin1String("auto_rotate_only_in_tablet_mode")) {
            output->set_auto_rotate_only_in_tablet_mode(value.toBool());
        } else if (key == QLatin1String("auto_resolution")) {
            output->set_auto_resolution(value.toBool());
        } else if (key == QLatin1String("auto_refresh_rate")) {
            output->set_auto_refresh_rate(value.toBool());
        } else if (key == QLatin1String("preferredModes")) {
            output->setPreferredModes(deserializeList<QString>(value.value<QDBusArgument>()));
        } else if (key == QLatin1String("followPreferredMode")) {
            output->setFollowPreferredMode(value.toBool());
        } else if (key == QLatin1String("enabled")) {
            output->setEnabled(value.toBool());
        } else if (key == QLatin1String("primary")) {
            output->setPrimary(value.toBool());
        } else if (key == QLatin1String("replicationSource")) {
            output->setReplicationSource(value.toInt());
        } else if (key == QLatin1String("sizeMM")) {
            output->setSizeMm(deserializeSize(value.value<QDBusArgument>()));
        } else if (key == QLatin1String("retention")) {
            output->set_retention(deserialize_retention(value));
        } else if (key == QLatin1String("modes")) {
            const QDBusArgument arg = value.value<QDBusArgument>();
            ModeList modes;
            arg.beginArray();
            while (!arg.atEnd()) {
                QVariant value;
                arg >> value;
                const Disman::ModePtr mode = deserializeMode(value.value<QDBusArgument>());
                if (!mode) {
                    return OutputPtr();
                }
                modes.insert(mode->id(), mode);
            }
            arg.endArray();
            output->setModes(modes);
        } else {
            qCWarning(DISMAN) << "Invalid key in Output map: " << key;
            return OutputPtr();
        }
        arg.endMapEntry();
    }
    arg.endMap();
    return output;
}

ModePtr ConfigSerializer::deserializeMode(const QDBusArgument& arg)
{
    ModePtr mode(new Mode);

    arg.beginMap();
    while (!arg.atEnd()) {
        QString key;
        QVariant value;
        arg.beginMapEntry();
        arg >> key >> value;

        if (key == QLatin1String("id")) {
            mode->setId(value.toString());
        } else if (key == QLatin1String("name")) {
            mode->setName(value.toString());
        } else if (key == QLatin1String("size")) {
            mode->setSize(deserializeSize(value.value<QDBusArgument>()));
        } else if (key == QLatin1String("refreshRate")) {
            mode->setRefreshRate(value.toFloat());
        } else {
            qCWarning(DISMAN) << "Invalid key in Mode map: " << key;
            return ModePtr();
        }
        arg.endMapEntry();
    }
    arg.endMap();
    return mode;
}

ScreenPtr ConfigSerializer::deserializeScreen(const QDBusArgument& arg)
{
    ScreenPtr screen(new Screen);

    arg.beginMap();
    QString key;
    QVariant value;
    while (!arg.atEnd()) {
        arg.beginMapEntry();
        arg >> key >> value;
        if (key == QLatin1String("id")) {
            screen->setId(value.toInt());
        } else if (key == QLatin1String("maxActiveOutputsCount")) {
            screen->setMaxActiveOutputsCount(value.toInt());
        } else if (key == QLatin1String("currentSize")) {
            screen->setCurrentSize(deserializeSize(value.value<QDBusArgument>()));
        } else if (key == QLatin1String("maxSize")) {
            screen->setMaxSize(deserializeSize(value.value<QDBusArgument>()));
        } else if (key == QLatin1String("minSize")) {
            screen->setMinSize(deserializeSize(value.value<QDBusArgument>()));
        } else {
            qCWarning(DISMAN) << "Invalid key in Screen map:" << key;
            return ScreenPtr();
        }
        arg.endMapEntry();
    }
    arg.endMap();
    return screen;
}
