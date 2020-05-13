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

#ifndef XRANDR11_BACKEND_H
#define XRANDR11_BACKEND_H

#include "abstractbackend.h"
#include "../xcbwrapper.h"

#include <QObject>
#include <QLoggingCategory>

class XCBEventListener;

class XRandR11 : public Disman::AbstractBackend
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kwinft.disman.backends.randr11")

public:
    explicit XRandR11();
    ~XRandR11() override;

    QString name() const override;
    QString serviceName() const override;
    Disman::ConfigPtr config() const override;
    void setConfig(const Disman::ConfigPtr &config) override;
    bool isValid() const override;

private Q_SLOTS:
    void updateConfig();

private:
    bool m_valid;
    XCBEventListener* m_x11Helper;
    Disman::ConfigPtr m_currentConfig;
    xcb_timestamp_t m_currentTimestamp;
};

Q_DECLARE_LOGGING_CATEGORY(DISMAN_XRANDR11)

#endif
