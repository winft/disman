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

#include "xrandr11.h"
#include "../xcbeventlistener.h"

#include "config.h"
#include "edid.h"
#include "output.h"

#include <xcb/xcb.h>
#include <xcb/randr.h>

#include <QString>
#include <QDebug>

Q_LOGGING_CATEGORY(DISMAN_XRANDR11, "disman.xrandr11")

XRandR11::XRandR11()
 : Disman::AbstractBackend()
 , m_valid(false)
 , m_x11Helper(nullptr)
 , m_currentConfig(new Disman::Config)
 , m_currentTimestamp(0)
{
    xcb_generic_error_t *error = nullptr;
    xcb_randr_query_version_reply_t* version;
    version = xcb_randr_query_version_reply(XCB::connection(),
        xcb_randr_query_version(XCB::connection(), XCB_RANDR_MAJOR_VERSION, XCB_RANDR_MINOR_VERSION), &error);

    if (!version || error) {
        free(error);
        XCB::closeConnection();
        qCDebug(DISMAN_XRANDR11) << "Can't get XRandR version";
        return;
    }
    if (version->major_version != 1 || version->minor_version != 1) {
        XCB::closeConnection();
        qCDebug(DISMAN_XRANDR11) << "This backend is only for XRandR 1.1, your version is: " << version->major_version << "." << version->minor_version;
        return;
    }

    m_x11Helper = new XCBEventListener();

    connect(m_x11Helper, &XCBEventListener::outputsChanged,
            this, &XRandR11::updateConfig);

    m_valid = true;
}

XRandR11::~XRandR11()
{
    XCB::closeConnection();
    delete m_x11Helper;
}

QString XRandR11::name() const
{
    return QStringLiteral("XRandR 1.1");
}

QString XRandR11::serviceName() const
{
    return QStringLiteral("org.kde.Disman.Backend.XRandR11");
}


bool XRandR11::isValid() const
{
    return m_valid;
}

Disman::ConfigPtr XRandR11::config() const
{
    Disman::ConfigPtr config(new Disman::Config);
    auto features = Disman::Config::Feature::Writable | Disman::Config::Feature::PrimaryDisplay;
    config->setSupportedFeatures(features);

    const int screenId = QX11Info::appScreen();
    xcb_screen_t* xcbScreen = XCB::screenOfDisplay(XCB::connection(), screenId);
    const XCB::ScreenInfo info(xcbScreen->root);
    const XCB::ScreenSize size(xcbScreen->root);

    if (info->config_timestamp == m_currentTimestamp) {
        return m_currentConfig;
    }

    Disman::ScreenPtr screen(new Disman::Screen);
    screen->setId(screenId);
    screen->setCurrentSize(QSize(xcbScreen->width_in_pixels, xcbScreen->height_in_pixels));
    if (size) { // RRGetScreenSize may file on VNC/RDP connections
        screen->setMaxSize(QSize(size->max_width, size->max_height));
        screen->setMinSize(QSize(size->min_width, size->min_height));
    } else {
        screen->setMaxSize(screen->currentSize());
        screen->setMinSize(screen->currentSize());
    }
    screen->setMaxActiveOutputsCount(1);

    config->setScreen(screen);

    Disman::OutputList outputs;
    Disman::OutputPtr output(new Disman::Output);
    output->setId(1);

    output->setConnected(true);
    output->setEnabled(true);
    output->setName(QStringLiteral("Default"));
    output->setPosition(QPoint(0,0));
    output->setPrimary(true);
    output->setRotation((Disman::Output::Rotation) info->rotation);
    output->setSizeMm(QSize(xcbScreen->width_in_millimeters, xcbScreen->height_in_millimeters));

    outputs.insert(1, output);
    config->setOutputs(outputs);

    Disman::ModePtr mode;
    Disman::ModeList modes;

    auto iter = xcb_randr_get_screen_info_rates_iterator(info);
    xcb_randr_screen_size_t* sizes = xcb_randr_get_screen_info_sizes(info);
    for (int x = 0; x < info->nSizes; x++) {
        const xcb_randr_screen_size_t size = sizes[x];
        const uint16_t* rates = xcb_randr_refresh_rates_rates(iter.data);
        const int nrates = xcb_randr_refresh_rates_rates_length(iter.data);

        for (int j = 0; j < nrates; j++) {
            float rate = rates[j];
            mode = Disman::ModePtr(new Disman::Mode);
            mode->setId(QStringLiteral("%1-%2").arg(x).arg(j));
            mode->setSize(QSize(size.width, size.height));
            mode->setRefreshRate(rate);
            mode->setName(QStringLiteral("%1x%2").arg(size.width).arg(size.height));

            if (x == info->sizeID && rate == info->rate) {
                output->setCurrentModeId(mode->id());
            }
            modes.insert(mode->id(), mode);
        }

        xcb_randr_refresh_rates_next(&iter);
    }

    output->setModes(modes);
    return config;
}

void XRandR11::setConfig(const Disman::ConfigPtr &config)
{
    const Disman::OutputPtr output = config->outputs().take(1);
    const Disman::ModePtr mode = output->currentMode();

    const int screenId = QX11Info::appScreen();
    xcb_screen_t* xcbScreen = XCB::screenOfDisplay(XCB::connection(), screenId);

    const XCB::ScreenInfo info(xcbScreen->root);
    xcb_generic_error_t *err;
    const int sizeId = mode->id().split(QLatin1Char('-')).first().toInt();
    auto cookie = xcb_randr_set_screen_config(XCB::connection(), xcbScreen->root,
                                         XCB_CURRENT_TIME, info->config_timestamp, sizeId,
                                         (short) output->rotation(), mode->refreshRate());
    XCB::ScopedPointer<xcb_randr_set_screen_config_reply_t> reply(
        xcb_randr_set_screen_config_reply(XCB::connection(), cookie, &err));
    if (err) {
        free(err);
    }
}

void XRandR11::updateConfig()
{
    m_currentConfig = config();
    Q_EMIT configChanged(m_currentConfig);
}
