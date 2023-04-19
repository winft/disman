/*************************************************************************************
 *  Copyright 2012, 2013  Daniel Vrátil <dvratil@redhat.com>                         *
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
#include "xcbeventlistener.h"

#include "xrandr_logging.h"

#include <QGuiApplication>
#include <QRect>
#include <QtGui/private/qtx11extras_p.h>

XCBEventListener::XCBEventListener()
    : m_isRandrPresent(false)
    , m_randrBase(0)
    , m_randrErrorBase(0)
    , m_majorOpcode(0)
    , m_versionMajor(0)
    , m_versionMinor(0)
    , m_window(0)
{
    xcb_connection_t* c = QX11Info::connection();
    xcb_prefetch_extension_data(c, &xcb_randr_id);
    auto cookie = xcb_randr_query_version(c, XCB_RANDR_MAJOR_VERSION, XCB_RANDR_MINOR_VERSION);
    const auto* queryExtension = xcb_get_extension_data(c, &xcb_randr_id);
    if (!queryExtension) {
        qCDebug(DISMAN_XRANDR) << "Fail to query for xrandr extension";
        return;
    }
    if (!queryExtension->present) {
        qCDebug(DISMAN_XRANDR) << "XRandR extension is not present at all";
        return;
    }

    m_isRandrPresent = queryExtension->present;
    m_randrBase = queryExtension->first_event;
    m_randrErrorBase = queryExtension->first_error;
    m_majorOpcode = queryExtension->major_opcode;

    xcb_generic_error_t* error = nullptr;
    auto* versionReply = xcb_randr_query_version_reply(c, cookie, &error);
    Q_ASSERT_X(versionReply, "xrandrxcbhelper", "Query to fetch xrandr version failed");
    if (error) {
        qFatal("Error while querying for xrandr version: %d", error->error_code);
    }
    m_versionMajor = versionReply->major_version;
    m_versionMinor = versionReply->minor_version;
    free(versionReply);

    qCDebug(DISMAN_XRANDR).nospace()
        << "Detected XRandR " << m_versionMajor << "." << m_versionMinor;
    qCDebug(DISMAN_XRANDR) << "Event Base: " << m_randrBase;
    qCDebug(DISMAN_XRANDR) << "Event Error: " << m_randrErrorBase;

    uint32_t rWindow = QX11Info::appRootWindow();
    m_window = xcb_generate_id(c);
    xcb_create_window(c,
                      XCB_COPY_FROM_PARENT,
                      m_window,
                      rWindow,
                      0,
                      0,
                      1,
                      1,
                      0,
                      XCB_COPY_FROM_PARENT,
                      XCB_COPY_FROM_PARENT,
                      0,
                      nullptr);

    xcb_randr_select_input(c,
                           m_window,
                           XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE | XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE
                               | XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE
                               | XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY);

    qApp->installNativeEventFilter(this);
}

XCBEventListener::~XCBEventListener()
{
    if (m_window && QX11Info::connection()) {
        xcb_destroy_window(QX11Info::connection(), m_window);
    }
}

QString XCBEventListener::rotationToString(xcb_randr_rotation_t rotation)
{
    switch (rotation) {
    case XCB_RANDR_ROTATION_ROTATE_0:
        return QStringLiteral("Rotate_0");
    case XCB_RANDR_ROTATION_ROTATE_90:
        return QStringLiteral("Rotate_90");
    case XCB_RANDR_ROTATION_ROTATE_180:
        return QStringLiteral("Rotate_180");
    case XCB_RANDR_ROTATION_ROTATE_270:
        return QStringLiteral("Rotate_270");
    case XCB_RANDR_ROTATION_REFLECT_X:
        return QStringLiteral("Reflect_X");
    case XCB_RANDR_ROTATION_REFLECT_Y:
        return QStringLiteral("Reflect_Y");
    }

    return QStringLiteral("invalid value (%1)").arg(rotation);
}

QString XCBEventListener::connectionToString(xcb_randr_connection_t connection)
{
    switch (connection) {
    case XCB_RANDR_CONNECTION_CONNECTED:
        return QStringLiteral("Connected");
    case XCB_RANDR_CONNECTION_DISCONNECTED:
        return QStringLiteral("Disconnected");
    case XCB_RANDR_CONNECTION_UNKNOWN:
        return QStringLiteral("UnknownConnection");
    }

    return QStringLiteral("invalid value (%1)").arg(connection);
}

bool XCBEventListener::nativeEventFilter(const QByteArray& eventType,
                                         void* message,
                                         qintptr* /*result*/)
{
    if (eventType != "xcb_generic_event_t") {
        return false;
    }

    auto* e = static_cast<xcb_generic_event_t*>(message);
    const uint8_t xEventType = e->response_type & ~0x80;

    // If this event is not xcb_randr_notify, we don't want it
    if (xEventType == m_randrBase + XCB_RANDR_SCREEN_CHANGE_NOTIFY) {
        handleScreenChange(e);
    }
    if (xEventType == m_randrBase + XCB_RANDR_NOTIFY) {
        handleXRandRNotify(e);
    }

    return false;
}

void XCBEventListener::handleScreenChange(xcb_generic_event_t* e)
{
    auto* e2 = reinterpret_cast<xcb_randr_screen_change_notify_event_t*>(e);

    // Only accept notifications for our window
    if (e2->request_window != m_window) {
        return;
    }

    qCDebug(DISMAN_XRANDR) << "RRScreenChangeNotify";
    qCDebug(DISMAN_XRANDR) << "\tWindow:" << e2->request_window;
    qCDebug(DISMAN_XRANDR) << "\tRoot:" << e2->root;
    qCDebug(DISMAN_XRANDR) << "\tRotation: "
                           << rotationToString((xcb_randr_rotation_t)e2->rotation);
    qCDebug(DISMAN_XRANDR) << "\tSize ID:" << e2->sizeID;
    qCDebug(DISMAN_XRANDR) << "\tSize: " << e2->width << e2->height;
    qCDebug(DISMAN_XRANDR) << "\tPhysical size: " << e2->mwidth << e2->mheight;

    Q_EMIT screenChanged((xcb_randr_rotation_t)e2->rotation,
                         QSize(e2->width, e2->height),
                         QSize(e2->mwidth, e2->mheight));
}

void XCBEventListener::handleXRandRNotify(xcb_generic_event_t* e)
{
    auto* randrEvent = reinterpret_cast<xcb_randr_notify_event_t*>(e);

    if (randrEvent->subCode == XCB_RANDR_NOTIFY_CRTC_CHANGE) {
        xcb_randr_crtc_change_t crtc = randrEvent->u.cc;
        qCDebug(DISMAN_XRANDR) << "RRNotify_CrtcChange";
        qCDebug(DISMAN_XRANDR) << "\tCRTC: " << crtc.crtc;
        qCDebug(DISMAN_XRANDR) << "\tMode: " << crtc.mode;
        qCDebug(DISMAN_XRANDR) << "\tRotation: "
                               << rotationToString((xcb_randr_rotation_t)crtc.rotation);
        qCDebug(DISMAN_XRANDR) << "\tGeometry: " << crtc.x << crtc.y << crtc.width << crtc.height;
        Q_EMIT crtcChanged(crtc.crtc,
                           crtc.mode,
                           (xcb_randr_rotation_t)crtc.rotation,
                           QRect(crtc.x, crtc.y, crtc.width, crtc.height));

    } else if (randrEvent->subCode == XCB_RANDR_NOTIFY_OUTPUT_CHANGE) {
        xcb_randr_output_change_t output = randrEvent->u.oc;
        qCDebug(DISMAN_XRANDR) << "RRNotify_OutputChange";
        qCDebug(DISMAN_XRANDR) << "\tOutput: " << output.output;
        qCDebug(DISMAN_XRANDR) << "\tCRTC: " << output.crtc;
        qCDebug(DISMAN_XRANDR) << "\tMode: " << output.mode;
        qCDebug(DISMAN_XRANDR) << "\tRotation: "
                               << rotationToString((xcb_randr_rotation_t)output.rotation);
        qCDebug(DISMAN_XRANDR) << "\tConnection: "
                               << connectionToString((xcb_randr_connection_t)output.connection);
        qCDebug(DISMAN_XRANDR) << "\tSubpixel Order: " << output.subpixel_order;
        Q_EMIT outputChanged(
            output.output, output.crtc, output.mode, (xcb_randr_connection_t)output.connection);

    } else if (randrEvent->subCode == XCB_RANDR_NOTIFY_OUTPUT_PROPERTY) {
        xcb_randr_output_property_t property = randrEvent->u.op;

        XCB::ScopedPointer<xcb_get_atom_name_reply_t> reply(
            xcb_get_atom_name_reply(QX11Info::connection(),
                                    xcb_get_atom_name(QX11Info::connection(), property.atom),
                                    nullptr));

        qCDebug(DISMAN_XRANDR) << "RRNotify_OutputProperty (ignored)";
        qCDebug(DISMAN_XRANDR) << "\tOutput: " << property.output;
        qCDebug(DISMAN_XRANDR) << "\tProperty: " << xcb_get_atom_name_name(reply.data());
        qCDebug(DISMAN_XRANDR) << "\tState (newValue, Deleted): " << property.status;
    }
}
