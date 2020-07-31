/*************************************************************************************
 *  Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *  Copyright (C) 2012, 2013 by Daniel Vrátil <dvratil@redhat.com>                   *
 *  Copyright 2014 Sebastian Kügler <sebas@kde.org>                                  *
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
#include "qscreenbackend.h"
#include "qscreenconfig.h"
#include "qscreenoutput.h"

using namespace Disman;

Q_LOGGING_CATEGORY(DISMAN_QSCREEN, "disman.qscreen")

QScreenConfig* QScreenBackend::s_internalConfig = nullptr;

QScreenBackend::QScreenBackend()
    : Disman::AbstractBackend()
    , m_isValid(true)
{
    if (s_internalConfig == nullptr) {
        s_internalConfig = new QScreenConfig();
        connect(
            s_internalConfig, &QScreenConfig::configChanged, this, &QScreenBackend::configChanged);
    }
}

QScreenBackend::~QScreenBackend()
{
}

QString QScreenBackend::name() const
{
    return QStringLiteral("QScreen");
}

QString QScreenBackend::serviceName() const
{
    return QStringLiteral("org.kde.Disman.Backend.QScreen");
}

ConfigPtr QScreenBackend::config() const
{
    return s_internalConfig->toDismanConfig();
}

void QScreenBackend::setConfig(const ConfigPtr& config)
{
    if (!config) {
        return;
    }

    qWarning() << "The QScreen backend for disman is read-only,";
    qWarning() << "setting a configuration is not supported.";
    qWarning() << "You can force another backend using the DISMAN_BACKEND env var.";
}

bool QScreenBackend::isValid() const
{
    return m_isValid;
}
