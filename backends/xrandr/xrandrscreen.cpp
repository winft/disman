/*************************************************************************************
 *  Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *  Copyright (C) 2012, 2013 by Daniel Vrátil <dvratil@redhat.com>                   *
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
#include "xrandrscreen.h"

#include "config.h"
#include "screen.h"
#include "xcbwrapper.h"
#include "xrandrconfig.h"

#include <QtGui/private/qtx11extras_p.h>

XRandRScreen::XRandRScreen(XRandRConfig* config)
    : QObject(config)
{
    XCB::ScreenSize size(XRandR::rootWindow());
    m_maxSize = QSize(size->max_width, size->max_height);
    m_minSize = QSize(size->min_width, size->min_height);
    update();
}

XRandRScreen::~XRandRScreen()
{
}

void XRandRScreen::update()
{
    xcb_screen_t* screen = XCB::screenOfDisplay(XCB::connection(), QX11Info::appScreen());
    m_currentSize = QSize(screen->width_in_pixels, screen->height_in_pixels);
}

void XRandRScreen::update(const QSize& size)
{
    m_currentSize = size;
}

QSize XRandRScreen::currentSize()
{
    return m_currentSize;
}

Disman::ScreenPtr XRandRScreen::toDismanScreen() const
{
    Disman::ScreenPtr dismanScreen(new Disman::Screen);
    dismanScreen->set_id(m_id);
    dismanScreen->set_max_size(m_maxSize);
    dismanScreen->set_min_size(m_minSize);
    dismanScreen->set_current_size(m_currentSize);

    XCB::ScopedPointer<xcb_randr_get_screen_resources_reply_t> screenResources(
        XRandR::screenResources());
    dismanScreen->set_max_outputs_count(screenResources->num_crtcs);

    return dismanScreen;
}

void XRandRScreen::updateDismanScreen(Disman::ScreenPtr& screen) const
{
    screen->set_current_size(m_currentSize);
}
