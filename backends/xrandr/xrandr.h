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
#pragma once

#include "abstractbackend.h"

#include <QLoggingCategory>
#include <QSize>

#include "xcbwrapper.h"

#include <memory>

class QRect;
class QTimer;

namespace Disman
{
class Filer_controller;
}

class XCBEventListener;
class XRandRConfig;

class XRandR : public Disman::AbstractBackend
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kwinft.disman.backends.randr")

public:
    explicit XRandR();
    ~XRandR() override;

    QString name() const override;
    QString service_name() const override;
    Disman::ConfigPtr config() const override;
    void set_config(const Disman::ConfigPtr& config) override;
    bool valid() const override;
    QByteArray edid(int outputId) const override;

    static QByteArray outputEdid(xcb_randr_output_t outputId);
    static xcb_randr_get_screen_resources_reply_t* screenResources();
    static xcb_screen_t* screen();
    static xcb_window_t rootWindow();

    static bool hasProperty(xcb_randr_output_t outputId, const QByteArray& name);

private:
    void outputChanged(xcb_randr_output_t output,
                       xcb_randr_crtc_t crtc,
                       xcb_randr_mode_t mode,
                       xcb_randr_connection_t connection);
    void crtcChanged(xcb_randr_crtc_t crtc,
                     xcb_randr_mode_t mode,
                     xcb_randr_rotation_t rotation,
                     const QRect& geom);
    void
    screenChanged(xcb_randr_rotation_t rotation, const QSize& sizePx, const QSize& physical_size);
    void handle_change();

    static quint8* getXProperty(xcb_randr_output_t output, xcb_atom_t atom, size_t& len);

    bool set_config_impl(Disman::ConfigPtr const& config);

    static xcb_screen_t* s_screen;
    static xcb_window_t s_rootWindow;
    static XRandRConfig* s_internalConfig;
    Disman::ConfigPtr m_config{nullptr};

    static int s_randrBase;
    static int s_randrError;
    static bool s_monitorInitialized;
    static bool s_has_1_3;
    static bool s_xorgCacheInitialized;

    XCBEventListener* m_x11Helper;
    bool m_valid;

    std::unique_ptr<Disman::Filer_controller> m_filer_controller;
    QTimer* m_configChangeCompressor;
};
