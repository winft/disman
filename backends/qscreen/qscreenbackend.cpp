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

QScreenConfig* QScreenBackend::s_internalConfig = nullptr;

QScreenBackend::QScreenBackend()
    : Disman::BackendImpl()
    , m_valid(true)
{
    if (s_internalConfig == nullptr) {
        s_internalConfig = new QScreenConfig();
        connect(s_internalConfig, &QScreenConfig::config_changed, this, [this] {
            Q_EMIT config_changed(config());
        });
    }
}

QScreenBackend::~QScreenBackend()
{
}

QString QScreenBackend::name() const
{
    return QStringLiteral("QScreen");
}

QString QScreenBackend::service_name() const
{
    return QStringLiteral("org.kde.Disman.Backend.QScreen");
}

void QScreenBackend::update_config(ConfigPtr& config) const
{
    s_internalConfig->update_config(config);
}

bool QScreenBackend::set_config_system([[maybe_unused]] const ConfigPtr& config)
{
    qWarning() << "The QScreen backend for disman is read-only,";
    qWarning() << "setting a configuration is not supported.";
    qWarning() << "You can force another backend using the DISMAN_BACKEND env var.";
    return false;
}

bool QScreenBackend::valid() const
{
    return m_valid;
}
