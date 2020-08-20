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
#include "fake.h"
#include "parser.h"

#include "fake_logging.h"

#include "../filer_controller.h"

#include "config.h"
#include <output.h>

#include <stdlib.h>

#include <QFile>
#include <QTimer>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <QDBusConnection>

#include "fakebackendadaptor.h"

using namespace Disman;

Fake::Fake()
    : Disman::AbstractBackend()
    , m_filer_controller{new Filer_controller}
{
    QLoggingCategory::setFilterRules(QStringLiteral("disman.fake.debug = true"));

    if (qgetenv("DISMAN_IN_PROCESS") != QByteArray("1")) {
        QTimer::singleShot(0, this, &Fake::delayedInit);
    }
}

void Fake::init(const QVariantMap& arguments)
{
    mConfig.reset();

    mConfigFile = arguments[QStringLiteral("TEST_DATA")].toString();
    qCDebug(DISMAN_FAKE) << "Fake profile file:" << mConfigFile;
}

void Fake::delayedInit()
{
    new FakebackendAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/fake"), this);
}

Fake::~Fake()
{
}

QString Fake::name() const
{
    return QStringLiteral("Fake");
}

QString Fake::service_name() const
{
    return QStringLiteral("org.kwinft.disman.fakebackend");
}

ConfigPtr Fake::config() const
{
    if (!mConfig) {
        mConfig = Parser::fromJson(mConfigFile);
        m_filer_controller->read(mConfig);
        mConfig = Parser::fromJson(mConfigFile);
    }

    return mConfig;
}

void Fake::set_config(const ConfigPtr& config)
{
    qCDebug(DISMAN_FAKE) << "set config" << config->outputs();
    m_filer_controller->write(config);
    mConfig = config->clone();
    emit config_changed(mConfig);
}

bool Fake::valid() const
{
    return true;
}

QByteArray Fake::edid(int outputId) const
{
    Q_UNUSED(outputId);
    QFile file(mConfigFile);
    file.open(QIODevice::ReadOnly);

    const QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll());
    const QJsonObject json = jsonDoc.object();

    const QJsonArray outputs = json[QStringLiteral("outputs")].toArray();
    Q_FOREACH (const QJsonValue& value, outputs) {
        const QVariantMap output = value.toObject().toVariantMap();
        if (output[QStringLiteral("id")].toInt() != outputId) {
            continue;
        }

        return QByteArray::fromBase64(output[QStringLiteral("edid")].toByteArray());
    }
    return QByteArray();
}

void Fake::setEnabled(int outputId, bool enabled)
{
    Disman::OutputPtr output = config()->output(outputId);
    if (output->enabled() == enabled) {
        return;
    }

    output->set_enabled(enabled);
    Q_EMIT config_changed(mConfig);
}

void Fake::setPrimary(int outputId, bool primary)
{
    auto output = config()->output(outputId);

    if (primary) {
        if (auto cur_prim = mConfig->primary_output()) {
            if (output == cur_prim) {
                return;
            }
            mConfig->set_primary_output(output);
        }
    } else {
        if (auto cur_prim = mConfig->primary_output()) {
            if (output != cur_prim) {
                return;
            }
            mConfig->set_primary_output(nullptr);
        }
    }

    Q_EMIT config_changed(mConfig);
}

void Fake::setCurrentModeId(int outputId, QString const& modeId)
{
    std::string const& string_mode_id = modeId.toStdString();
    auto output = config()->output(outputId);

    if (auto mode = output->commanded_mode(); mode && mode->id() == string_mode_id) {
        return;
    }

    output->set_mode(output->mode(string_mode_id));
    Q_EMIT config_changed(mConfig);
}

void Fake::setRotation(int outputId, int rotation)
{
    Disman::OutputPtr output = config()->output(outputId);
    const Disman::Output::Rotation rot = static_cast<Disman::Output::Rotation>(rotation);
    if (output->rotation() == rot) {
        return;
    }

    output->set_rotation(rot);
    Q_EMIT config_changed(mConfig);
}

void Fake::addOutput(int outputId, const QString& name)
{
    Disman::OutputPtr output(new Disman::Output);
    output->set_id(outputId);
    output->set_name(name.toStdString());
    output->set_description(name.toStdString());
    output->set_hash(name.toStdString());
    mConfig->add_output(output);
    Q_EMIT config_changed(mConfig);
}

void Fake::removeOutput(int outputId)
{
    mConfig->remove_output(outputId);
    Q_EMIT config_changed(mConfig);
}
