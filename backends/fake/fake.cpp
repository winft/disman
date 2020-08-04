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
#include "edid.h"
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

    if (qgetenv("DISMAN_BACKEND_INPROCESS") != QByteArray("1")) {
        QTimer::singleShot(0, this, &Fake::delayedInit);
    }
}

void Fake::init(const QVariantMap& arguments)
{
    if (!mConfig.isNull()) {
        mConfig.clear();
    }

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

QString Fake::serviceName() const
{
    return QStringLiteral("org.kwinft.disman.fakebackend");
}

ConfigPtr Fake::config() const
{
    if (mConfig.isNull()) {
        mConfig = Parser::fromJson(mConfigFile);
        m_filer_controller->read(mConfig);
        mConfig = Parser::fromJson(mConfigFile);
    }

    return mConfig;
}

void Fake::setConfig(const ConfigPtr& config)
{
    qCDebug(DISMAN_FAKE) << "set config" << config->outputs();
    m_filer_controller->write(config);
    mConfig = config->clone();
    emit configChanged(mConfig);
}

bool Fake::isValid() const
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
    if (output->isEnabled() == enabled) {
        return;
    }

    output->setEnabled(enabled);
    Q_EMIT configChanged(mConfig);
}

void Fake::setPrimary(int outputId, bool primary)
{
    Disman::OutputPtr output = config()->output(outputId);
    if (output->isPrimary() == primary) {
        return;
    }

    Q_FOREACH (Disman::OutputPtr output, config()->outputs()) {
        if (output->id() == outputId) {
            output->setPrimary(primary);
        } else {
            output->setPrimary(false);
        }
    }
    Q_EMIT configChanged(mConfig);
}

void Fake::setCurrentModeId(int outputId, const QString& modeId)
{
    Disman::OutputPtr output = config()->output(outputId);
    if (auto mode = output->commanded_mode(); mode && mode->id() == modeId) {
        return;
    }

    output->set_mode(output->mode(modeId));
    Q_EMIT configChanged(mConfig);
}

void Fake::setRotation(int outputId, int rotation)
{
    Disman::OutputPtr output = config()->output(outputId);
    const Disman::Output::Rotation rot = static_cast<Disman::Output::Rotation>(rotation);
    if (output->rotation() == rot) {
        return;
    }

    output->setRotation(rot);
    Q_EMIT configChanged(mConfig);
}

void Fake::addOutput(int outputId, const QString& name)
{
    Disman::OutputPtr output(new Disman::Output);
    output->setId(outputId);
    output->setName(name);
    mConfig->addOutput(output);
    Q_EMIT configChanged(mConfig);
}

void Fake::removeOutput(int outputId)
{
    mConfig->removeOutput(outputId);
    Q_EMIT configChanged(mConfig);
}
