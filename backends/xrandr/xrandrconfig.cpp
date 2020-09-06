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
#include "xcbwrapper.h"
#include "xrandr.h"
#include "xrandrcrtc.h"
#include "xrandrmode.h"
#include "xrandroutput.h"
#include "xrandrscreen.h"

#include "xrandr_logging.h"

#include "config.h"
#include "output.h"

#include <QRect>

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
    for (auto& [key, output] : m_outputs) {
        delete output;
    }
    for (auto& [key, crtc] : m_crtcs) {
        delete crtc;
    }
    delete m_screen;
}

XRandROutput::Map XRandRConfig::outputs() const
{
    return m_outputs;
}

XRandROutput* XRandRConfig::output(xcb_randr_output_t output) const
{
    if (auto it = m_outputs.find(output); it != m_outputs.end()) {
        return it->second;
    }
    return nullptr;
}

XRandRCrtc::Map XRandRConfig::crtcs() const
{
    return m_crtcs;
}

XRandRCrtc* XRandRConfig::crtc(xcb_randr_crtc_t crtc) const
{
    if (auto it = m_crtcs.find(crtc); it != m_crtcs.end()) {
        return it->second;
    }
    return nullptr;
}

XRandRScreen* XRandRConfig::screen() const
{
    return m_screen;
}

void XRandRConfig::addNewOutput(xcb_randr_output_t id)
{
    XRandROutput* xOutput = new XRandROutput(id, this);
    m_outputs.insert({id, xOutput});
}

void XRandRConfig::addNewCrtc(xcb_randr_crtc_t crtc)
{
    m_crtcs.insert({crtc, new XRandRCrtc(crtc, this)});
}

void XRandRConfig::removeOutput(xcb_randr_output_t id)
{
    auto it = m_outputs.find(id);
    if (it != m_outputs.end()) {
        delete it->second;
        m_outputs.erase(it);
    }
}

Disman::ConfigPtr XRandRConfig::update_config(Disman::ConfigPtr& config) const
{
    const Config::Features features = Config::Feature::Writable | Config::Feature::PrimaryDisplay
        | Config::Feature::OutputReplication;
    config->set_supported_features(features);

    Disman::OutputList dismanOutputs;

    for (auto const& [key, output] : m_outputs) {
        if (!output->isConnected()) {
            continue;
        }

        Disman::OutputPtr dismanOutput;
        if (auto existing_output = config->output(output->id())) {
            dismanOutput = existing_output;
        } else {
            dismanOutput.reset(new Disman::Output);
        }

        output->updateDismanOutput(dismanOutput);
        dismanOutputs.insert({dismanOutput->id(), dismanOutput});
    }

    config->set_outputs(dismanOutputs);
    config->setScreen(m_screen->toDismanScreen());

    return config;
}

bool XRandRConfig::applyDismanConfig(const Disman::ConfigPtr& config)
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

    for (auto const& [key, xrandrOutput] : m_outputs) {
        if (xrandrOutput->isPrimary()) {
            oldPrimaryOutput = xrandrOutput->id();
            break;
        }
    }

    Disman::OutputList toDisable, toEnable, toChange;

    // Only set the output as primary if it is enabled.
    if (auto primary = config->primary_output(); primary && primary->enabled()) {
        primaryOutput = primary->id();
    }

    for (auto const& [key, dismanOutput] : dismanOutputs) {
        xcb_randr_output_t outputId = dismanOutput->id();
        XRandROutput* currentOutput = output(outputId);

        const bool currentEnabled = currentOutput->enabled();

        if (!dismanOutput->enabled() && currentEnabled) {
            toDisable.insert({outputId, dismanOutput});
            continue;
        } else if (dismanOutput->enabled() && !currentEnabled) {
            toEnable.insert({outputId, dismanOutput});
            ++neededCrtcs;
            continue;
        } else if (!dismanOutput->enabled() && !currentEnabled) {
            continue;
        }

        ++neededCrtcs;

        if (dismanOutput->auto_mode()->id() != currentOutput->currentModeId()) {
            if (toChange.find(outputId) == toChange.end()) {
                toChange.insert({outputId, dismanOutput});
            }
        }

        if (dismanOutput->position() != currentOutput->position()) {
            if (toChange.find(outputId) == toChange.end()) {
                toChange.insert({outputId, dismanOutput});
            }
        }

        if (dismanOutput->rotation() != currentOutput->rotation()) {
            if (toChange.find(outputId) == toChange.end()) {
                toChange.insert({outputId, dismanOutput});
            }
        }

        if (dismanOutput->geometry().size() != currentOutput->logicalSize()) {
            if (toChange.find(outputId) == toChange.end()) {
                toChange.insert({outputId, dismanOutput});
            }
        }

        auto currentMode = currentOutput->modes().at(std::stoi(dismanOutput->auto_mode()->id()));
        // For some reason, in some environments currentMode is null
        // which doesn't make sense because it is the *current* mode...
        // Since we haven't been able to figure out the reason why
        // this happens, we are adding this debug code to try to
        // figure out how this happened.
        if (!currentMode) {
            qWarning() << "Current mode is null:"
                       << "ModeId:" << currentOutput->currentModeId().c_str()
                       << "Mode: " << currentOutput->currentMode()
                       << "Output: " << currentOutput->id();
            printConfig(config);
            printInternalCond();
            continue;
        }

        // When the output would not fit into new screen size, we need to disable and reposition it.
        const QRectF geom = dismanOutput->geometry();
        if (geom.right() > newScreenSize.width() || geom.bottom() > newScreenSize.height()) {
            if (toDisable.find(outputId) == toDisable.end()) {
                qCDebug(DISMAN_XRANDR)
                    << "The new output would not fit into screen - new geometry: " << geom
                    << ", new screen size:" << newScreenSize;
                toDisable.insert({outputId, dismanOutput});
            }
        }
    }

    const Disman::ScreenPtr dismanScreen = config->screen();
    if (newScreenSize.width() > dismanScreen->max_size().width()
        || newScreenSize.height() > dismanScreen->max_size().height()) {
        qCDebug(DISMAN_XRANDR) << "The new screen size is too big - requested: " << newScreenSize
                               << ", maximum: " << dismanScreen->max_size();
        return false;
    }

    qCDebug(DISMAN_XRANDR) << "Needed CRTCs: " << neededCrtcs;

    XCB::ScopedPointer<xcb_randr_get_screen_resources_reply_t> screenResources(
        XRandR::screenResources());

    if (neededCrtcs > screenResources->num_crtcs) {
        qCDebug(DISMAN_XRANDR) << "We need more CRTCs than we have available - requested: "
                               << neededCrtcs << ", available: " << screenResources->num_crtcs;
        return false;
    }

    qCDebug(DISMAN_XRANDR) << "Actions to perform:";
    qCDebug(DISMAN_XRANDR) << "\tPrimary Output:" << (primaryOutput != oldPrimaryOutput);
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

    auto print_keys = [](auto const& outputs) {
        std::string line = "\t\t";
        for (auto const& [key, output] : outputs) {
            line = line + " " + std::to_string(key);
        }
        qCDebug(DISMAN_XRANDR) << line.c_str();
    };

    qCDebug(DISMAN_XRANDR) << "\tDisable outputs:" << !toDisable.empty();
    if (!toDisable.empty()) {
        print_keys(toDisable);
    }

    qCDebug(DISMAN_XRANDR) << "\tChange outputs:" << !toChange.empty();
    if (!toChange.empty()) {
        print_keys(toChange);
    }

    qCDebug(DISMAN_XRANDR) << "\tEnable outputs:" << !toEnable.empty();
    if (!toEnable.empty()) {
        print_keys(toEnable);
    }

    // Grab the server so that no-one else can do changes to XRandR and to block
    // change notifications until we are done
    XCB::GrabServer grabber;

    // If there is nothing to do, not even bother
    if (oldPrimaryOutput == primaryOutput && toDisable.empty() && toEnable.empty()
        && toChange.empty()) {
        if (newScreenSize != currentScreenSize) {
            setScreenSize(newScreenSize);
        }
        return false;
    }

    for (auto const& [key, output] : toDisable) {
        disableOutput(output);
    }

    if (intermediateScreenSize != currentScreenSize) {
        setScreenSize(intermediateScreenSize);
    }

    bool forceScreenSizeUpdate = false;

    for (auto const& [key, output] : toChange) {
        if (!changeOutput(output, output->id() == static_cast<int>(primaryOutput))) {
            /* If we disabled the output before changing it and XRandR failed
             * to re-enable it, then update screen size too */
            if (toDisable.find(output->id()) != toDisable.end()) {
                output->set_enabled(false);
                qCDebug(DISMAN_XRANDR) << "Output failed to change: " << output->name().c_str();
                forceScreenSizeUpdate = true;
            }
        }
    }

    for (auto const& [key, output] : toEnable) {
        if (!enableOutput(output, output->id() == static_cast<int>(primaryOutput))) {
            qCDebug(DISMAN_XRANDR) << "Output failed to be Enabled: " << output->name().c_str();
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

    return true;
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
                           << "\tmax_size:" << config->screen()->max_size() << "\n"
                           << "\tmin_size:" << config->screen()->min_size() << "\n"
                           << "\tcurrent_size:" << config->screen()->current_size();

    qCDebug(DISMAN_XRANDR) << "Primary output:"
                           << (config->primary_output()
                                   ? std::to_string(config->primary_output()->id()).c_str()
                                   : "none");

    const OutputList outputs = config->outputs();
    for (auto const& [key, output] : outputs) {
        qCDebug(DISMAN_XRANDR) << "\n-----------------------------------------------------\n"
                               << "\n"
                               << "Id: " << output->id() << "\n"
                               << "Connector name: " << output->name().c_str() << "\n"
                               << "Description: " << output->description().c_str() << "\n"
                               << "Type: " << output->type();

        qCDebug(DISMAN_XRANDR) << "Enabled: " << output->enabled() << "\n"
                               << "Rotation: " << output->rotation() << "\n"
                               << "Pos: " << output->position() << "\n"
                               << "Phyiscal size: " << output->physical_size();
        if (output->auto_mode()) {
            qCDebug(DISMAN_XRANDR) << "Size: " << output->auto_mode()->size();
        }

        qCDebug(DISMAN_XRANDR) << "Mode: " << output->auto_mode()->id().c_str() << "\n"
                               << "Preferred Mode: " << output->preferred_mode()->id().c_str()
                               << "\n"
                               << "Preferred modes:";
        for (auto const& mode_string : output->preferred_modes()) {
            qCDebug(DISMAN_XRANDR) << "\t" << mode_string.c_str();
        }

        qCDebug(DISMAN_XRANDR) << "Modes: ";
        for (auto const& [key, mode] : output->modes()) {
            qCDebug(DISMAN_XRANDR) << "\t" << mode->id().c_str() << "  " << mode->name().c_str()
                                   << " " << mode->size() << " " << mode->refresh();
        }
    }
}

void XRandRConfig::printInternalCond() const
{
    qCDebug(DISMAN_XRANDR) << "Internal config in xrandr";
    for (auto const& [key, output] : m_outputs) {
        qCDebug(DISMAN_XRANDR) << "Id: " << output->id() << "\n"
                               << "Current Mode: " << output->currentMode() << "\n"
                               << "Current mode id: " << output->currentModeId().c_str() << "\n"
                               << "Connected: " << output->isConnected() << "\n"
                               << "Enabled: " << output->enabled() << "\n"
                               << "Primary: " << output->isPrimary();
        if (!output->enabled()) {
            continue;
        }

        XRandRMode::Map modes = output->modes();
        for (auto const& [mode_key, mode] : modes) {
            qCDebug(DISMAN_XRANDR) << "\t" << mode->id() << "\n"
                                   << "\t" << mode->name() << "\n"
                                   << "\t" << mode->size() << mode->refreshRate();
        }
    }
}

QSize XRandRConfig::screenSize(const Disman::ConfigPtr& config) const
{
    QRect rect;
    for (auto const& [key, output] : config->outputs()) {
        if (!output->enabled()) {
            continue;
        }

        const ModePtr currentMode = output->auto_mode();
        if (!currentMode) {
            qCDebug(DISMAN_XRANDR)
                << "Output: " << output->name().c_str() << " has no current Mode!";
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
                           << "\tPhysical size:" << QSize(widthMM, heightMM);

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

    for (auto const& [key, output] : m_outputs) {
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
                        false);
    }
    return (reply->status == XCB_RANDR_SET_CONFIG_SUCCESS);
}

bool XRandRConfig::enableOutput(const OutputPtr& dismanOutput, bool primary) const
{
    XRandRCrtc* freeCrtc = nullptr;
    qCDebug(DISMAN_XRANDR) << m_crtcs;

    for (auto const& [key, crtc] : m_crtcs) {
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
    auto const modeId = std::stoi(dismanOutput->auto_mode() ? dismanOutput->auto_mode()->id()
                                                            : dismanOutput->preferred_mode()->id());
    xOutput->updateLogicalSize(dismanOutput, freeCrtc);

    qCDebug(DISMAN_XRANDR) << "RRSetCrtcConfig (enable output)"
                           << "\n"
                           << "\tOutput:" << dismanOutput->id() << "("
                           << dismanOutput->name().c_str() << ")"
                           << "\n"
                           << "\tNew CRTC:" << freeCrtc->crtc() << "\n"
                           << "\tPos:" << dismanOutput->position() << "\n"
                           << "\tMode:" << dismanOutput->auto_mode()
                           << "Preferred:" << dismanOutput->preferred_mode()->id().c_str() << "\n"
                           << "\tRotation:" << dismanOutput->rotation();

    if (!sendConfig(dismanOutput, freeCrtc)) {
        return false;
    }

    xOutput->update(freeCrtc->crtc(), modeId, XCB_RANDR_CONNECTION_CONNECTED, primary);
    return true;
}

bool XRandRConfig::changeOutput(const Disman::OutputPtr& dismanOutput, bool primary) const
{
    XRandROutput* xOutput = output(dismanOutput->id());
    Q_ASSERT(xOutput);

    if (!xOutput->crtc()) {
        qCDebug(DISMAN_XRANDR) << "Output" << dismanOutput->id()
                               << "has no CRTC, falling back to enableOutput()";
        return enableOutput(dismanOutput, primary);
    }

    auto const modeId = std::stoi(dismanOutput->auto_mode() ? dismanOutput->auto_mode()->id()
                                                            : dismanOutput->preferred_mode()->id());
    xOutput->updateLogicalSize(dismanOutput);

    qCDebug(DISMAN_XRANDR) << "RRSetCrtcConfig (change output)"
                           << "\n"
                           << "\tOutput:" << dismanOutput->id() << "("
                           << dismanOutput->name().c_str() << ")"
                           << "\n"
                           << "\tCRTC:" << xOutput->crtc()->crtc() << "\n"
                           << "\tPos:" << dismanOutput->position() << "\n"
                           << "\tMode:" << modeId << dismanOutput->auto_mode() << "\n"
                           << "\tRotation:" << dismanOutput->rotation();

    if (!sendConfig(dismanOutput, xOutput->crtc())) {
        return false;
    }

    xOutput->update(xOutput->crtc()->crtc(), modeId, XCB_RANDR_CONNECTION_CONNECTED, primary);
    return true;
}

bool XRandRConfig::sendConfig(const Disman::OutputPtr& dismanOutput, XRandRCrtc* crtc) const
{
    xcb_randr_output_t outputs[1]{static_cast<xcb_randr_output_t>(dismanOutput->id())};
    auto const modeId = std::stoi(dismanOutput->auto_mode() ? dismanOutput->auto_mode()->id()
                                                            : dismanOutput->preferred_mode()->id());

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
