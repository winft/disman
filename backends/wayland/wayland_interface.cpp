/*************************************************************************
Copyright © 2020   Roman Gilg <subdiff@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
**************************************************************************/
#include "wayland_interface.h"

#include "waylandbackend.h"
#include "waylandoutput.h"
#include "waylandscreen.h"

#include <configmonitor.h>
#include <mode.h>

#include "wayland_logging.h"

#include <QThread>
#include <QTimer>

using namespace Disman;

WaylandInterface::WaylandInterface(QObject* parent)
    : QObject(parent)
    , m_blockSignals(true)
    , m_dismanConfig(new Config)
{
}

WaylandInterface::~WaylandInterface() = default;

bool WaylandInterface::signalsBlocked() const
{
    return m_blockSignals;
}

void WaylandInterface::blockSignals()
{
    Q_ASSERT(m_blockSignals == false);
    m_blockSignals = true;
}

void WaylandInterface::unblockSignals()
{
    Q_ASSERT(m_blockSignals == true);
    m_blockSignals = false;
}

void WaylandInterface::handleDisconnect()
{
    qCWarning(DISMAN_WAYLAND) << "Wayland disconnected, cleaning up.";
    Q_EMIT configChanged();
}

void WaylandInterface::addOutput(WaylandOutput* output)
{
    m_initializingOutputs << output;

    connect(output, &WaylandOutput::removed, this, [this, output]() { removeOutput(output); });
    connect(output, &WaylandOutput::dataReceived, this, [this, output]() { initOutput(output); });
}

void WaylandInterface::initOutput(WaylandOutput* output)
{
    insertOutput(output);
    m_initializingOutputs.removeOne(output);
    checkInitialized();

    if (!signalsBlocked() && m_initializingOutputs.empty()) {
        Q_EMIT outputsChanged();
        Q_EMIT configChanged();
    }

    connect(output, &WaylandOutput::changed, this, [this]() {
        if (!signalsBlocked()) {
            Q_EMIT configChanged();
        }
    });
}

void WaylandInterface::removeOutput(WaylandOutput* output)
{
    if (m_initializingOutputs.removeOne(output)) {
        // Output was not yet fully initialized, just remove here and return.
        delete output;
        return;
    }

    // remove the output from output mapping
    const auto removedOutput = takeOutput(output);
    Q_ASSERT(removedOutput == output);
    Q_UNUSED(removedOutput);
    Q_EMIT outputsChanged();
    delete output;

    if (!m_blockSignals) {
        Q_EMIT configChanged();
    }
}

void WaylandInterface::checkInitialized()
{
    if (isInitialized()) {
        Q_EMIT initialized();
    }
}

bool WaylandInterface::isInitialized() const
{
    return !m_blockSignals && m_initializingOutputs.isEmpty();
}
