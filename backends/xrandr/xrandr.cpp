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
#include "xrandr.h"

#include "xcbeventlistener.h"
#include "xcbwrapper.h"
#include "xrandrconfig.h"
#include "xrandrscreen.h"

#include "xrandr_logging.h"

#include "../filer_controller.h"

#include "config.h"
#include "generator.h"
#include "output.h"

#include <QRect>
#include <QTime>
#include <QTimer>

#include <QX11Info>

xcb_screen_t* XRandR::s_screen = nullptr;
xcb_window_t XRandR::s_rootWindow = 0;
XRandRConfig* XRandR::s_internalConfig = nullptr;
int XRandR::s_randrBase = 0;
int XRandR::s_randrError = 0;
bool XRandR::s_monitorInitialized = false;
bool XRandR::s_has_1_3 = false;
bool XRandR::s_xorgCacheInitialized = false;

using namespace Disman;

XRandR::XRandR()
    : Disman::AbstractBackend()
    , m_x11Helper(nullptr)
    , m_valid(false)
    , m_configChangeCompressor(nullptr)
{
    qRegisterMetaType<xcb_randr_output_t>("xcb_randr_output_t");
    qRegisterMetaType<xcb_randr_crtc_t>("xcb_randr_crtc_t");
    qRegisterMetaType<xcb_randr_mode_t>("xcb_randr_mode_t");
    qRegisterMetaType<xcb_randr_connection_t>("xcb_randr_connection_t");
    qRegisterMetaType<xcb_randr_rotation_t>("xcb_randr_rotation_t");

    // Use our own connection to make sure that we won't mess up Qt's connection
    // if something goes wrong on our side.
    xcb_generic_error_t* error = nullptr;
    xcb_randr_query_version_reply_t* version;
    XCB::connection();
    version = xcb_randr_query_version_reply(XCB::connection(),
                                            xcb_randr_query_version(XCB::connection(),
                                                                    XCB_RANDR_MAJOR_VERSION,
                                                                    XCB_RANDR_MINOR_VERSION),
                                            &error);
    if (!version || error) {
        XCB::closeConnection();
        free(error);
        return;
    }

    if ((version->major_version > 1)
        || ((version->major_version == 1) && (version->minor_version >= 2))) {
        m_valid = true;
    } else {
        XCB::closeConnection();
        qCWarning(DISMAN_XRANDR) << "XRandR extension not available or unsupported version";
        return;
    }

    m_filer_controller.reset(new Filer_controller);

    if (s_screen == nullptr) {
        s_screen = XCB::screenOfDisplay(XCB::connection(), QX11Info::appScreen());
        s_rootWindow = s_screen->root;

        xcb_prefetch_extension_data(XCB::connection(), &xcb_randr_id);
        auto reply = xcb_get_extension_data(XCB::connection(), &xcb_randr_id);
        s_randrBase = reply->first_event;
        s_randrError = reply->first_error;
    }

    XRandR::s_has_1_3 = (version->major_version > 1
                         || (version->major_version == 1 && version->minor_version >= 3));

    if (s_internalConfig == nullptr) {
        s_internalConfig = new XRandRConfig();
    }

    if (!s_monitorInitialized) {
        m_x11Helper = new XCBEventListener();
        connect(m_x11Helper,
                &XCBEventListener::outputChanged,
                this,
                &XRandR::outputChanged,
                Qt::QueuedConnection);
        connect(m_x11Helper,
                &XCBEventListener::crtcChanged,
                this,
                &XRandR::crtcChanged,
                Qt::QueuedConnection);
        connect(m_x11Helper,
                &XCBEventListener::screenChanged,
                this,
                &XRandR::screenChanged,
                Qt::QueuedConnection);

        m_configChangeCompressor = new QTimer(this);
        m_configChangeCompressor->setSingleShot(true);
        m_configChangeCompressor->setInterval(500);
        connect(m_configChangeCompressor, &QTimer::timeout, this, &XRandR::handle_change);

        handle_change();
        s_monitorInitialized = true;
    }
}

XRandR::~XRandR()
{
    delete m_x11Helper;
}

QString XRandR::name() const
{
    return QStringLiteral("XRandR");
}

QString XRandR::service_name() const
{
    return QStringLiteral("org.kde.Disman.Backend.XRandR");
}

void XRandR::outputChanged(xcb_randr_output_t output,
                           xcb_randr_crtc_t crtc,
                           xcb_randr_mode_t mode,
                           xcb_randr_connection_t connection)
{
    m_configChangeCompressor->start();

    auto xOutput = s_internalConfig->output(output);

    if (connection == XCB_RANDR_CONNECTION_DISCONNECTED) {
        if (xOutput) {
            xOutput->disconnected();
            s_internalConfig->removeOutput(output);
            qCDebug(DISMAN_XRANDR) << "Output" << output << " removed";
        }
        return;
    }

    if (!xOutput) {
        s_internalConfig->addNewOutput(output);
        return;
    }

    XCB::PrimaryOutput primary(XRandR::rootWindow());
    xOutput->update(crtc, mode, connection, (primary->output == output));
    qCDebug(DISMAN_XRANDR) << "Output" << xOutput->id() << ": connected =" << xOutput->isConnected()
                           << ", enabled =" << xOutput->enabled();
}

void XRandR::crtcChanged(xcb_randr_crtc_t crtc,
                         xcb_randr_mode_t mode,
                         xcb_randr_rotation_t rotation,
                         const QRect& geom)
{
    XRandRCrtc* xCrtc = s_internalConfig->crtc(crtc);
    if (!xCrtc) {
        s_internalConfig->addNewCrtc(crtc);
    } else {
        xCrtc->update(mode, rotation, geom);
    }

    m_configChangeCompressor->start();
}

void XRandR::handle_change()
{
    auto cfg = config();
    if (!m_config || m_config->hash() != cfg->hash()) {
        qCDebug(DISMAN_XRANDR) << "Config with new output pattern received:" << cfg;

        if (cfg->origin() == Config::Origin::unknown) {
            qCDebug(DISMAN_XRANDR)
                << "Config received that is unknown. Creating an optimized config now.";
            Generator generator(cfg, m_config);
            generator.optimize();
            cfg = generator.config();
        } else {
            m_filer_controller->read(cfg);
        }

        m_config = cfg;

        if (set_config_impl(cfg)) {
            qCDebug(DISMAN_XRANDR) << "Config for new output pattern sent.";
            return;
        }
    }
    Q_EMIT config_changed(cfg);
}

void XRandR::screenChanged(xcb_randr_rotation_t rotation,
                           const QSize& sizePx,
                           const QSize& physical_size)
{
    Q_UNUSED(physical_size);

    QSize newSizePx = sizePx;
    if (rotation == XCB_RANDR_ROTATION_ROTATE_90 || rotation == XCB_RANDR_ROTATION_ROTATE_270) {
        newSizePx.transpose();
    }

    XRandRScreen* xScreen = s_internalConfig->screen();
    Q_ASSERT(xScreen);
    xScreen->update(newSizePx);

    m_configChangeCompressor->start();
}

// TODO: read from control file!

ConfigPtr XRandR::config() const
{
    Disman::ConfigPtr config(new Disman::Config);

    s_internalConfig->update_config(config);
    m_filer_controller->read(config);
    s_internalConfig->update_config(config);

    return config;
}

void XRandR::set_config(const ConfigPtr& config)
{
    if (!config) {
        return;
    }
    set_config_impl(config);
}

bool XRandR::set_config_impl(Disman::ConfigPtr const& config)
{
    m_filer_controller->write(config);
    return s_internalConfig->applyDismanConfig(config);
}

QByteArray XRandR::edid(int outputId) const
{
    const XRandROutput* output = s_internalConfig->output(outputId);
    if (!output) {
        return QByteArray();
    }

    return output->edid();
}

bool XRandR::valid() const
{
    return m_valid;
}

quint8* XRandR::getXProperty(xcb_randr_output_t output, xcb_atom_t atom, size_t& len)
{
    quint8* result;

    auto cookie = xcb_randr_get_output_property(
        XCB::connection(), output, atom, XCB_ATOM_ANY, 0, 100, false, false);
    auto reply = xcb_randr_get_output_property_reply(XCB::connection(), cookie, nullptr);

    if (reply->type == XCB_ATOM_INTEGER && reply->format == 8) {
        result = new quint8[reply->num_items];
        memcpy(result, xcb_randr_get_output_property_data(reply), reply->num_items);
        len = reply->num_items;
    } else {
        result = nullptr;
    }

    free(reply);
    return result;
}

QByteArray XRandR::outputEdid(xcb_randr_output_t outputId)
{
    size_t len = 0;
    quint8* result;

    auto edid_atom = XCB::InternAtom(false, 4, "EDID")->atom;
    result = XRandR::getXProperty(outputId, edid_atom, len);
    if (result == nullptr) {
        auto edid_atom = XCB::InternAtom(false, 9, "EDID_DATA")->atom;
        result = XRandR::getXProperty(outputId, edid_atom, len);
    }
    if (result == nullptr) {
        auto edid_atom = XCB::InternAtom(false, 25, "XFree86_DDC_EDID1_RAWDATA")->atom;
        result = XRandR::getXProperty(outputId, edid_atom, len);
    }

    QByteArray edid;
    if (result != nullptr) {
        if (len % 128 == 0) {
            edid = QByteArray(reinterpret_cast<const char*>(result), len);
        }
        delete[] result;
    }
    return edid;
}

bool XRandR::hasProperty(xcb_randr_output_t output, const QByteArray& name)
{
    xcb_generic_error_t* error = nullptr;
    auto atom = XCB::InternAtom(false, name.length(), name.constData())->atom;

    auto cookie = xcb_randr_get_output_property(
        XCB::connection(), output, atom, XCB_ATOM_ANY, 0, 1, false, false);
    auto prop_reply = xcb_randr_get_output_property_reply(XCB::connection(), cookie, &error);

    const bool ret = prop_reply->num_items == 1;
    free(prop_reply);
    return ret;
}

xcb_randr_get_screen_resources_reply_t* XRandR::screenResources()
{
    if (XRandR::s_has_1_3) {
        if (XRandR::s_xorgCacheInitialized) {
            // HACK: This abuses the fact that xcb_randr_get_screen_resources_reply_t
            // and xcb_randr_get_screen_resources_current_reply_t are the same
            return reinterpret_cast<xcb_randr_get_screen_resources_reply_t*>(
                xcb_randr_get_screen_resources_current_reply(
                    XCB::connection(),
                    xcb_randr_get_screen_resources_current(XCB::connection(), XRandR::rootWindow()),
                    nullptr));
        } else {
            /* XRRGetScreenResourcesCurrent is faster then XRRGetScreenResources
             * because it returns cached values. However the cached values are not
             * available until someone calls XRRGetScreenResources first. In case
             * we happen to be the first ones, we need to fill the cache first. */
            XRandR::s_xorgCacheInitialized = true;
        }
    }

    return xcb_randr_get_screen_resources_reply(
        XCB::connection(),
        xcb_randr_get_screen_resources(XCB::connection(), XRandR::rootWindow()),
        nullptr);
}

xcb_window_t XRandR::rootWindow()
{
    return s_rootWindow;
}

xcb_screen_t* XRandR::screen()
{
    return s_screen;
}
