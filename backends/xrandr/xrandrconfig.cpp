/*************************************************************************************
 *  Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *  Copyright (C) 2012 - 2015 by Daniel Vrátil <dvratil@redhat.com>                  *
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
#include "xrandrconfig.h"
#include "config.h"
#include "edid.h"
#include "output.h"
#include "xrandr.h"
#include "xrandrcrtc.h"
#include "xrandrmode.h"
#include "xrandroutput.h"
#include "xrandrscreen.h"

#include "xcbwrapper.h"

#include <QRect>
#include <QScopedPointer>
#include <QX11Info>

using namespace Disman;

XRandRConfig::XRandRConfig()
    : QObject()
    , m_screen(nullptr)
{
    m_screen = new XRandRScreen(this);

    XCB::ScopedPointer<xcb_randr_get_screen_resources_reply_t> resources(XRandR::screenResources());

    xcb_randr_crtc_t* crtcs = xcb_randr_get_screen_resources_crtcs(resources.data());
    const int crtcsCount = xcb_randr_get_screen_resources_crtcs_length(resources.data());
    for (int i = 0; i < crtcsCount; ++i) {
        addNewCrtc(crtcs[i]);
    }

    xcb_randr_output_t* outputs = xcb_randr_get_screen_resources_outputs(resources.data());
    const int outputsCount = xcb_randr_get_screen_resources_outputs_length(resources.data());
    for (int i = 0; i < outputsCount; ++i) {
        addNewOutput(outputs[i]);
    }
}

XRandRConfig::~XRandRConfig()
{
    qDeleteAll(m_outputs);
    qDeleteAll(m_crtcs);
    delete m_screen;
}

XRandROutput::Map XRandRConfig::outputs() const
{
    return m_outputs;
}

XRandROutput* XRandRConfig::output(xcb_randr_output_t output) const
{
    return m_outputs[output];
}

XRandRCrtc::Map XRandRConfig::crtcs() const
{
    return m_crtcs;
}

XRandRCrtc* XRandRConfig::crtc(xcb_randr_crtc_t crtc) const
{
    return m_crtcs[crtc];
}

XRandRScreen* XRandRConfig::screen() const
{
    return m_screen;
}

void XRandRConfig::addNewOutput(xcb_randr_output_t id)
{
    XRandROutput* xOutput = new XRandROutput(id, this);
    m_outputs.insert(id, xOutput);
}

void XRandRConfig::addNewCrtc(xcb_randr_crtc_t crtc)
{
    m_crtcs.insert(crtc, new XRandRCrtc(crtc, this));
}

void XRandRConfig::removeOutput(xcb_randr_output_t id)
{
    delete m_outputs.take(id);
}

Disman::ConfigPtr XRandRConfig::toDismanConfig() const
{
    Disman::ConfigPtr config(new Disman::Config);

    const Config::Features features = Config::Feature::Writable | Config::Feature::PrimaryDisplay
        | Config::Feature::OutputReplication;
    config->setSupportedFeatures(features);

    Disman::OutputList dismanOutputs;

    for (auto iter = m_outputs.constBegin(); iter != m_outputs.constEnd(); ++iter) {
        Disman::OutputPtr dismanOutput = (*iter)->toDismanOutput();
        dismanOutputs.insert(dismanOutput->id(), dismanOutput);
    }

    config->setOutputs(dismanOutputs);
    config->setScreen(m_screen->toDismanScreen());

    return config;
}

void XRandRConfig::applyDismanConfig(const Disman::ConfigPtr& config)
{
    const Disman::OutputList dismanOutputs = config->outputs();

    const QSize newScreenSize = screenSize(config);
    const QSize currentScreenSize = m_screen->currentSize();

    // When the current screen configuration is bigger than the new size (like
    // when rotating an output), the XSetScreenSize can fail or apply the smaller
    // size only partially, because we apply the size (we have to) before the
    // output changes. To prevent all kinds of weird screen sizes from happening,
    // we initially set such screen size, that it can take the current as well
    // as the new configuration, then we apply the output changes, and finally then
    // (if necessary) we reduce the screen size to fix the new configuration precisely.
    const QSize intermediateScreenSize
        = QSize(qMax(newScreenSize.width(), currentScreenSize.width()),
                qMax(newScreenSize.height(), currentScreenSize.height()));

    int neededCrtcs = 0;
    xcb_randr_output_t primaryOutput = 0;
    xcb_randr_output_t oldPrimaryOutput = 0;

    for (const XRandROutput* xrandrOutput : m_outputs) {
        if (xrandrOutput->isPrimary()) {
            oldPrimaryOutput = xrandrOutput->id();
            break;
        }
    }

    Disman::OutputList toDisable, toEnable, toChange;

    for (const Disman::OutputPtr& dismanOutput : dismanOutputs) {
        xcb_randr_output_t outputId = dismanOutput->id();
        XRandROutput* currentOutput = output(outputId);
        // Only set the output as primary if it is enabled.
        if (dismanOutput->isPrimary() && dismanOutput->isEnabled()) {
            primaryOutput = outputId;
        }

        const bool currentEnabled = currentOutput->isEnabled();

        if (!dismanOutput->isEnabled() && currentEnabled) {
            toDisable.insert(outputId, dismanOutput);
            continue;
        } else if (dismanOutput->isEnabled() && !currentEnabled) {
            toEnable.insert(outputId, dismanOutput);
            ++neededCrtcs;
            continue;
        } else if (!dismanOutput->isEnabled() && !currentEnabled) {
            continue;
        }

        ++neededCrtcs;

        if (dismanOutput->currentModeId() != currentOutput->currentModeId()) {
            if (!toChange.contains(outputId)) {
                toChange.insert(outputId, dismanOutput);
            }
        }

        if (dismanOutput->position() != currentOutput->position()) {
            if (!toChange.contains(outputId)) {
                toChange.insert(outputId, dismanOutput);
            }
        }

        if (dismanOutput->rotation() != currentOutput->rotation()) {
            if (!toChange.contains(outputId)) {
                toChange.insert(outputId, dismanOutput);
            }
        }

        if (dismanOutput->geometry().size() != currentOutput->logicalSize()) {
            if (!toChange.contains(outputId)) {
                toChange.insert(outputId, dismanOutput);
            }
        }

        XRandRMode* currentMode
            = currentOutput->modes().value(dismanOutput->currentModeId().toInt());
        // For some reason, in some environments currentMode is null
        // which doesn't make sense because it is the *current* mode...
        // Since we haven't been able to figure out the reason why
        // this happens, we are adding this debug code to try to
        // figure out how this happened.
        if (!currentMode) {
            qWarning() << "Current mode is null:"
                       << "ModeId:" << currentOutput->currentModeId()
                       << "Mode: " << currentOutput->currentMode()
                       << "Output: " << currentOutput->id();
            printConfig(config);
            printInternalCond();
            continue;
        }

        // When the output would not fit into new screen size, we need to disable and reposition it.
        const QRectF geom = dismanOutput->geometry();
        if (geom.right() > newScreenSize.width() || geom.bottom() > newScreenSize.height()) {
            if (!toDisable.contains(outputId)) {
                qCDebug(DISMAN_XRANDR)
                    << "The new output would not fit into screen - new geometry: " << geom
                    << ", new screen size:" << newScreenSize;
                toDisable.insert(outputId, dismanOutput);
            }
        }
    }

    const Disman::ScreenPtr dismanScreen = config->screen();
    if (newScreenSize.width() > dismanScreen->maxSize().width()
        || newScreenSize.height() > dismanScreen->maxSize().height()) {
        qCDebug(DISMAN_XRANDR) << "The new screen size is too big - requested: " << newScreenSize
                               << ", maximum: " << dismanScreen->maxSize();
        return;
    }

    qCDebug(DISMAN_XRANDR) << "Needed CRTCs: " << neededCrtcs;

    XCB::ScopedPointer<xcb_randr_get_screen_resources_reply_t> screenResources(
        XRandR::screenResources());

    if (neededCrtcs > screenResources->num_crtcs) {
        qCDebug(DISMAN_XRANDR) << "We need more CRTCs than we have available - requested: "
                               << neededCrtcs << ", available: " << screenResources->num_crtcs;
        return;
    }

    qCDebug(DISMAN_XRANDR) << "Actions to perform:"
                           << "\n"
                           << "\tPrimary Output:" << (primaryOutput != oldPrimaryOutput);
    if (primaryOutput != oldPrimaryOutput) {
        qCDebug(DISMAN_XRANDR) << "\t\tOld:" << oldPrimaryOutput << "\n"
                               << "\t\tNew:" << primaryOutput;
    }

    qCDebug(DISMAN_XRANDR) << "\tChange Screen Size:" << (newScreenSize != currentScreenSize);
    if (newScreenSize != currentScreenSize) {
        qCDebug(DISMAN_XRANDR) << "\t\tOld:" << currentScreenSize << "\n"
                               << "\t\tIntermediate:" << intermediateScreenSize << "\n"
                               << "\t\tNew:" << newScreenSize;
    }

    qCDebug(DISMAN_XRANDR) << "\tDisable outputs:" << !toDisable.isEmpty();
    if (!toDisable.isEmpty()) {
        qCDebug(DISMAN_XRANDR) << "\t\t" << toDisable.keys();
    }

    qCDebug(DISMAN_XRANDR) << "\tChange outputs:" << !toChange.isEmpty();
    if (!toChange.isEmpty()) {
        qCDebug(DISMAN_XRANDR) << "\t\t" << toChange.keys();
    }

    qCDebug(DISMAN_XRANDR) << "\tEnable outputs:" << !toEnable.isEmpty();
    if (!toEnable.isEmpty()) {
        qCDebug(DISMAN_XRANDR) << "\t\t" << toEnable.keys();
    }

    // Grab the server so that no-one else can do changes to XRandR and to block
    // change notifications until we are done
    XCB::GrabServer grabber;

    // If there is nothing to do, not even bother
    if (oldPrimaryOutput == primaryOutput && toDisable.isEmpty() && toEnable.isEmpty()
        && toChange.isEmpty()) {
        if (newScreenSize != currentScreenSize) {
            setScreenSize(newScreenSize);
        }
        return;
    }

    for (const Disman::OutputPtr& output : toDisable) {
        disableOutput(output);
    }

    if (intermediateScreenSize != currentScreenSize) {
        setScreenSize(intermediateScreenSize);
    }

    bool forceScreenSizeUpdate = false;

    for (const Disman::OutputPtr& output : toChange) {
        if (!changeOutput(output)) {
            /* If we disabled the output before changing it and XRandR failed
             * to re-enable it, then update screen size too */
            if (toDisable.contains(output->id())) {
                output->setEnabled(false);
                qCDebug(DISMAN_XRANDR) << "Output failed to change: " << output->name();
                forceScreenSizeUpdate = true;
            }
        }
    }

    for (const Disman::OutputPtr& output : toEnable) {
        if (!enableOutput(output)) {
            qCDebug(DISMAN_XRANDR) << "Output failed to be Enabled: " << output->name();
            forceScreenSizeUpdate = true;
        }
    }

    if (oldPrimaryOutput != primaryOutput) {
        setPrimaryOutput(primaryOutput);
    }

    if (forceScreenSizeUpdate || intermediateScreenSize != newScreenSize) {
        QSize newSize = newScreenSize;
        if (forceScreenSizeUpdate) {
            newSize = screenSize(config);
            qCDebug(DISMAN_XRANDR) << "Forced to change screen size: " << newSize;
        }
        setScreenSize(newSize);
    }
}

void XRandRConfig::printConfig(const ConfigPtr& config) const
{
    qCDebug(DISMAN_XRANDR) << "Disman version:" /*<< DISMAN_VERSION*/;

    if (!config) {
        qCDebug(DISMAN_XRANDR) << "Config is invalid";
        return;
    }
    if (!config->screen()) {
        qCDebug(DISMAN_XRANDR) << "No screen in the configuration, broken backend";
        return;
    }

    qCDebug(DISMAN_XRANDR) << "Screen:"
                           << "\n"
                           << "\tmaxSize:" << config->screen()->maxSize() << "\n"
                           << "\tminSize:" << config->screen()->minSize() << "\n"
                           << "\tcurrentSize:" << config->screen()->currentSize();

    const OutputList outputs = config->outputs();
    for (const OutputPtr& output : outputs) {
        qCDebug(DISMAN_XRANDR) << "\n-----------------------------------------------------\n"
                               << "\n"
                               << "Id: " << output->id() << "\n"
                               << "Name: " << output->name() << "\n"
                               << "Type: " << output->type() << "\n"
                               << "Connected: " << output->isConnected();

        if (!output->isConnected()) {
            continue;
        }

        qCDebug(DISMAN_XRANDR) << "Enabled: " << output->isEnabled() << "\n"
                               << "Primary: " << output->isPrimary() << "\n"
                               << "Rotation: " << output->rotation() << "\n"
                               << "Pos: " << output->position() << "\n"
                               << "MMSize: " << output->sizeMm();
        if (output->currentMode()) {
            qCDebug(DISMAN_XRANDR) << "Size: " << output->currentMode()->size();
        }

        qCDebug(DISMAN_XRANDR) << "Clones: "
                               << (output->clones().isEmpty()
                                       ? QStringLiteral("None")
                                       : QString::number(output->clones().count()))
                               << "\n"
                               << "Mode: " << output->currentModeId() << "\n"
                               << "Preferred Mode: " << output->preferredModeId() << "\n"
                               << "Preferred modes: " << output->preferredModes() << "\n"
                               << "Modes: ";

        ModeList modes = output->modes();
        for (const ModePtr& mode : modes) {
            qCDebug(DISMAN_XRANDR) << "\t" << mode->id() << "  " << mode->name() << " "
                                   << mode->size() << " " << mode->refreshRate();
        }

        Edid* edid = output->edid();
        qCDebug(DISMAN_XRANDR) << "EDID Info: ";
        if (edid && edid->isValid()) {
            qCDebug(DISMAN_XRANDR) << "\tDevice ID: " << edid->deviceId() << "\n"
                                   << "\tName: " << edid->name() << "\n"
                                   << "\tVendor: " << edid->vendor() << "\n"
                                   << "\tSerial: " << edid->serial() << "\n"
                                   << "\tEISA ID: " << edid->eisaId() << "\n"
                                   << "\tHash: " << edid->hash() << "\n"
                                   << "\tWidth: " << edid->width() << "\n"
                                   << "\tHeight: " << edid->height() << "\n"
                                   << "\tGamma: " << edid->gamma() << "\n"
                                   << "\tRed: " << edid->red() << "\n"
                                   << "\tGreen: " << edid->green() << "\n"
                                   << "\tBlue: " << edid->blue() << "\n"
                                   << "\tWhite: " << edid->white();
        } else {
            qCDebug(DISMAN_XRANDR) << "\tUnavailable";
        }
    }
}

void XRandRConfig::printInternalCond() const
{
    qCDebug(DISMAN_XRANDR) << "Internal config in xrandr";
    for (const XRandROutput* output : m_outputs) {
        qCDebug(DISMAN_XRANDR) << "Id: " << output->id() << "\n"
                               << "Current Mode: " << output->currentMode() << "\n"
                               << "Current mode id: " << output->currentModeId() << "\n"
                               << "Connected: " << output->isConnected() << "\n"
                               << "Enabled: " << output->isEnabled() << "\n"
                               << "Primary: " << output->isPrimary();
        if (!output->isEnabled()) {
            continue;
        }

        XRandRMode::Map modes = output->modes();
        for (const XRandRMode* mode : modes) {
            qCDebug(DISMAN_XRANDR) << "\t" << mode->id() << "\n"
                                   << "\t" << mode->name() << "\n"
                                   << "\t" << mode->size() << mode->refreshRate();
        }
    }
}

QSize XRandRConfig::screenSize(const Disman::ConfigPtr& config) const
{
    QRect rect;
    for (const Disman::OutputPtr& output : config->outputs()) {
        if (!output->isConnected() || !output->isEnabled()) {
            continue;
        }

        const ModePtr currentMode = output->currentMode();
        if (!currentMode) {
            qCDebug(DISMAN_XRANDR) << "Output: " << output->name() << " has no current Mode!";
            continue;
        }

        const QRect outputGeom = output->geometry().toRect();
        rect = rect.united(outputGeom);
    }

    const QSize size = QSize(rect.width(), rect.height());
    qCDebug(DISMAN_XRANDR) << "Requested screen size is" << size;
    return size;
}

bool XRandRConfig::setScreenSize(const QSize& size) const
{
    const double dpi
        = 25.4 * XRandR::screen()->height_in_pixels / XRandR::screen()->height_in_millimeters;
    const int widthMM = (25.4 * size.width()) / dpi;
    const int heightMM = (25.4 * size.height()) / dpi;

    qCDebug(DISMAN_XRANDR) << "RRSetScreenSize"
                           << "\n"
                           << "\tDPI:" << dpi << "\n"
                           << "\tSize:" << size << "\n"
                           << "\tSizeMM:" << QSize(widthMM, heightMM);

    xcb_randr_set_screen_size(
        XCB::connection(), XRandR::rootWindow(), size.width(), size.height(), widthMM, heightMM);
    m_screen->update(size);
    return true;
}

void XRandRConfig::setPrimaryOutput(xcb_randr_output_t outputId) const
{
    qCDebug(DISMAN_XRANDR) << "RRSetOutputPrimary"
                           << "\n"
                           << "\tNew primary:" << outputId;
    xcb_randr_set_output_primary(XCB::connection(), XRandR::rootWindow(), outputId);

    for (XRandROutput* output : m_outputs) {
        output->setIsPrimary(output->id() == outputId);
    }
}

bool XRandRConfig::disableOutput(const OutputPtr& dismanOutput) const
{
    XRandROutput* xOutput = output(dismanOutput->id());
    Q_ASSERT(xOutput);
    Q_ASSERT(xOutput->crtc());

    if (!xOutput->crtc()) {
        qCWarning(DISMAN_XRANDR) << "Attempting to disable output without CRTC, wth?";
        return false;
    }

    const xcb_randr_crtc_t crtc = xOutput->crtc()->crtc();

    qCDebug(DISMAN_XRANDR) << "RRSetCrtcConfig (disable output)"
                           << "\n"
                           << "\tCRTC:" << crtc;

    auto cookie = xcb_randr_set_crtc_config(XCB::connection(),
                                            crtc,
                                            XCB_CURRENT_TIME,
                                            XCB_CURRENT_TIME,
                                            0,
                                            0,
                                            XCB_NONE,
                                            XCB_RANDR_ROTATION_ROTATE_0,
                                            0,
                                            nullptr);

    XCB::ScopedPointer<xcb_randr_set_crtc_config_reply_t> reply(
        xcb_randr_set_crtc_config_reply(XCB::connection(), cookie, nullptr));

    if (!reply) {
        qCDebug(DISMAN_XRANDR) << "\tResult: unknown (error)";
        return false;
    }
    qCDebug(DISMAN_XRANDR) << "\tResult:" << reply->status;

    // Update the cached output now, otherwise we get RRNotify_CrtcChange notification
    // for an outdated output, which can lead to a crash.
    if (reply->status == XCB_RANDR_SET_CONFIG_SUCCESS) {
        xOutput->update(XCB_NONE,
                        XCB_NONE,
                        xOutput->isConnected() ? XCB_RANDR_CONNECTION_CONNECTED
                                               : XCB_RANDR_CONNECTION_DISCONNECTED,
                        dismanOutput->isPrimary());
    }
    return (reply->status == XCB_RANDR_SET_CONFIG_SUCCESS);
}

bool XRandRConfig::enableOutput(const OutputPtr& dismanOutput) const
{
    XRandRCrtc* freeCrtc = nullptr;
    qCDebug(DISMAN_XRANDR) << m_crtcs;

    for (XRandRCrtc* crtc : m_crtcs) {
        crtc->update();
        qCDebug(DISMAN_XRANDR) << "Testing CRTC" << crtc->crtc() << "\n"
                               << "\tFree:" << crtc->isFree() << "\n"
                               << "\tMode:" << crtc->mode() << "\n"
                               << "\tPossible outputs:" << crtc->possibleOutputs() << "\n"
                               << "\tConnected outputs:" << crtc->outputs() << "\n"
                               << "\tGeometry:" << crtc->geometry();

        if (crtc->isFree() && crtc->possibleOutputs().contains(dismanOutput->id())) {
            freeCrtc = crtc;
            break;
        }
    }

    if (!freeCrtc) {
        qCWarning(DISMAN_XRANDR) << "Failed to get free CRTC for output" << dismanOutput->id();
        return false;
    }

    XRandROutput* xOutput = output(dismanOutput->id());
    const int modeId = dismanOutput->currentMode() ? dismanOutput->currentModeId().toInt()
                                                   : dismanOutput->preferredModeId().toInt();
    xOutput->updateLogicalSize(dismanOutput, freeCrtc);

    qCDebug(DISMAN_XRANDR) << "RRSetCrtcConfig (enable output)"
                           << "\n"
                           << "\tOutput:" << dismanOutput->id() << "(" << dismanOutput->name()
                           << ")"
                           << "\n"
                           << "\tNew CRTC:" << freeCrtc->crtc() << "\n"
                           << "\tPos:" << dismanOutput->position() << "\n"
                           << "\tMode:" << dismanOutput->currentMode()
                           << "Preferred:" << dismanOutput->preferredModeId() << "\n"
                           << "\tRotation:" << dismanOutput->rotation();

    if (!sendConfig(dismanOutput, freeCrtc)) {
        return false;
    }

    xOutput->update(
        freeCrtc->crtc(), modeId, XCB_RANDR_CONNECTION_CONNECTED, dismanOutput->isPrimary());
    return true;
}

bool XRandRConfig::changeOutput(const Disman::OutputPtr& dismanOutput) const
{
    XRandROutput* xOutput = output(dismanOutput->id());
    Q_ASSERT(xOutput);

    if (!xOutput->crtc()) {
        qCDebug(DISMAN_XRANDR) << "Output" << dismanOutput->id()
                               << "has no CRTC, falling back to enableOutput()";
        return enableOutput(dismanOutput);
    }

    int modeId = dismanOutput->currentMode() ? dismanOutput->currentModeId().toInt()
                                             : dismanOutput->preferredModeId().toInt();
    xOutput->updateLogicalSize(dismanOutput);

    qCDebug(DISMAN_XRANDR) << "RRSetCrtcConfig (change output)"
                           << "\n"
                           << "\tOutput:" << dismanOutput->id() << "(" << dismanOutput->name()
                           << ")"
                           << "\n"
                           << "\tCRTC:" << xOutput->crtc()->crtc() << "\n"
                           << "\tPos:" << dismanOutput->position() << "\n"
                           << "\tMode:" << modeId << dismanOutput->currentMode() << "\n"
                           << "\tRotation:" << dismanOutput->rotation();

    if (!sendConfig(dismanOutput, xOutput->crtc())) {
        return false;
    }

    xOutput->update(
        xOutput->crtc()->crtc(), modeId, XCB_RANDR_CONNECTION_CONNECTED, dismanOutput->isPrimary());
    return true;
}

bool XRandRConfig::sendConfig(const Disman::OutputPtr& dismanOutput, XRandRCrtc* crtc) const
{
    xcb_randr_output_t outputs[1]{static_cast<xcb_randr_output_t>(dismanOutput->id())};
    const int modeId = dismanOutput->currentMode() ? dismanOutput->currentModeId().toInt()
                                                   : dismanOutput->preferredModeId().toInt();

    auto cookie = xcb_randr_set_crtc_config(XCB::connection(),
                                            crtc->crtc(),
                                            XCB_CURRENT_TIME,
                                            XCB_CURRENT_TIME,
                                            dismanOutput->position().rx(),
                                            dismanOutput->position().ry(),
                                            modeId,
                                            dismanOutput->rotation(),
                                            1,
                                            outputs);

    XCB::ScopedPointer<xcb_randr_set_crtc_config_reply_t> reply(
        xcb_randr_set_crtc_config_reply(XCB::connection(), cookie, nullptr));
    if (!reply) {
        qCDebug(DISMAN_XRANDR) << "\tResult: unknown (error)";
        return false;
    }
    qCDebug(DISMAN_XRANDR) << "\tResult: " << reply->status;
    return (reply->status == XCB_RANDR_SET_CONFIG_SUCCESS);
}
