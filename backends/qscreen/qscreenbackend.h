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
#ifndef QSCREEN_BACKEND_H
#define QSCREEN_BACKEND_H

#include "abstractbackend.h"

#include <QLoggingCategory>

namespace Disman
{
class QScreenConfig;

class QScreenBackend : public Disman::AbstractBackend
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kwinft.disman.backends.qscreen")

public:
    explicit QScreenBackend();
    ~QScreenBackend() override;

    QString name() const override;
    QString serviceName() const override;
    Disman::ConfigPtr config() const override;
    void setConfig(const Disman::ConfigPtr& config) override;
    bool isValid() const override;

private:
    bool m_isValid;
    static QScreenConfig* s_internalConfig;
};

}

Q_DECLARE_LOGGING_CATEGORY(DISMAN_QSCREEN)

#endif
