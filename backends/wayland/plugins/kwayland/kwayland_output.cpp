/*************************************************************************
Copyright © 2014-2015 Sebastian Kügler <sebas@kde.org>
Copyright © 2019-2020 Roman Gilg <subdiff@gmail.com>

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
#include "kwayland_output.h"

#include <mode.h>

#include "kwayland_logging.h"

#include <KWayland/Client/outputconfiguration.h>
#include <KWayland/Client/outputdevice.h>

using namespace Disman;
namespace Wl = KWayland::Client;

const std::map<Wl::OutputDevice::Transform, Output::Rotation> s_rotationMap
    = {{Wl::OutputDevice::Transform::Normal, Output::None},
       {Wl::OutputDevice::Transform::Rotated90, Output::Right},
       {Wl::OutputDevice::Transform::Rotated180, Output::Inverted},
       {Wl::OutputDevice::Transform::Rotated270, Output::Left},
       {Wl::OutputDevice::Transform::Flipped, Output::None},
       {Wl::OutputDevice::Transform::Flipped90, Output::Right},
       {Wl::OutputDevice::Transform::Flipped180, Output::Inverted},
       {Wl::OutputDevice::Transform::Flipped270, Output::Left}};

Output::Rotation toDismanRotation(const Wl::OutputDevice::Transform transform)
{
    auto it = s_rotationMap.find(transform);
    assert(it != s_rotationMap.end());
    return it->second;
}

Wl::OutputDevice::Transform toKWaylandTransform(const Output::Rotation rotation)
{
    for (auto const& [key, val] : s_rotationMap) {
        if (val == rotation) {
            return key;
        }
    }
    assert(false);
}

KWaylandOutput::KWaylandOutput(quint32 id, QObject* parent)
    : WaylandOutput(id, parent)
    , m_device(nullptr)
{
}

bool KWaylandOutput::enabled() const
{
    return m_device != nullptr;
}

QByteArray KWaylandOutput::edid() const
{
    return m_device->edid();
}

QRectF KWaylandOutput::geometry() const
{
    return m_device->geometry();
}

Wl::OutputDevice* KWaylandOutput::outputDevice() const
{
    return m_device;
}

void KWaylandOutput::createOutputDevice(Wl::Registry* registry, quint32 name, quint32 version)
{
    Q_ASSERT(!m_device);
    m_device = registry->createOutputDevice(name, version);

    connect(m_device, &Wl::OutputDevice::removed, this, &KWaylandOutput::removed);
    connect(m_device, &Wl::OutputDevice::done, this, [this]() {
        Q_EMIT dataReceived();
        connect(m_device, &Wl::OutputDevice::changed, this, &KWaylandOutput::changed);
    });
}

void KWaylandOutput::updateDismanOutput(OutputPtr& output)
{
    // Initialize primary output
    output->set_enabled(m_device->enabled() == Wl::OutputDevice::Enablement::Enabled);
    output->set_name(name().toStdString());
    output->set_description(name().toStdString());
    output->set_hash(name().toStdString());
    output->set_physical_size(m_device->physicalSize());
    output->set_position(m_device->globalPosition());
    output->set_rotation(s_rotationMap.at(m_device->transform()));

    ModeMap modeList;
    std::vector<std::string> preferredModeIds;
    m_modeIdMap.clear();
    ModePtr current_mode;

    for (const Wl::OutputDevice::Mode& wlMode : m_device->modes()) {
        ModePtr mode(new Mode());
        auto const name = modeName(wlMode).toStdString();

        auto modeId = std::to_string(wlMode.id);
        if (modeId.empty()) {
            qCWarning(DISMAN_WAYLAND) << "Could not create mode id from" << wlMode.id << ", using"
                                      << name.c_str() << "instead.";
            modeId = name;
        }

        if (m_modeIdMap.find(modeId) != m_modeIdMap.end()) {
            qCWarning(DISMAN_WAYLAND) << "Mode id already in use:" << modeId.c_str();
        }
        mode->set_id(modeId);

        // KWayland gives the refresh rate as int in mHz
        mode->set_refresh(wlMode.refreshRate / 1000.0);
        mode->set_size(wlMode.size);
        mode->set_name(name);

        if (wlMode.flags.testFlag(Wl::OutputDevice::Mode::Flag::Current)) {
            current_mode = mode;
        }
        if (wlMode.flags.testFlag(Wl::OutputDevice::Mode::Flag::Preferred)) {
            preferredModeIds.push_back(modeId);
        }

        // Update the disman => kwayland mode id translation map
        m_modeIdMap.insert({modeId, wlMode.id});
        // Add to the modelist which gets set on the output
        modeList[modeId] = mode;
    }

    output->set_preferred_modes(preferredModeIds);
    output->set_modes(modeList);

    if (!current_mode) {
        qCWarning(DISMAN_WAYLAND) << "Could not find the current mode. Available modes:";
        for (auto const& [key, mode] : modeList) {
            qCWarning(DISMAN_WAYLAND) << "  " << mode;
        }
    } else {
        output->set_mode(current_mode);
        output->set_resolution(current_mode->size());
        auto success = output->set_refresh_rate(current_mode->refresh());
        if (!success) {
            qCWarning(DISMAN_WAYLAND) << "Failed setting the current mode:" << current_mode;
        }
    }
    output->set_scale(m_device->scaleF());
    output->setType(guessType(m_device->model(), m_device->model()));
}

bool KWaylandOutput::setWlConfig(Wl::OutputConfiguration* wlConfig, const Disman::OutputPtr& output)
{
    bool changed = false;

    // enabled?
    if ((m_device->enabled() == Wl::OutputDevice::Enablement::Enabled) != output->enabled()) {
        changed = true;
        const auto enablement = output->enabled() ? Wl::OutputDevice::Enablement::Enabled
                                                  : Wl::OutputDevice::Enablement::Disabled;
        wlConfig->setEnabled(m_device, enablement);
    }

    // position
    if (m_device->globalPosition() != output->position()) {
        changed = true;
        wlConfig->setPosition(m_device, output->position().toPoint());
    }

    // scale
    if (!qFuzzyCompare(m_device->scaleF(), output->scale())) {
        changed = true;
        wlConfig->setScaleF(m_device, output->scale());
    }

    // rotation
    if (toDismanRotation(m_device->transform()) != output->rotation()) {
        changed = true;
        wlConfig->setTransform(m_device, toKWaylandTransform(output->rotation()));
    }

    // mode
    if (auto mode = output->auto_mode();
        mode && m_modeIdMap.find(mode->id()) != m_modeIdMap.end()) {
        auto const newModeId = m_modeIdMap.at(mode->id());
        if (newModeId != m_device->currentMode().id) {
            changed = true;
            wlConfig->setMode(m_device, newModeId);
        }
    } else {
        qCWarning(DISMAN_WAYLAND) << "Invalid Disman mode:" << (mode ? mode->id() : "null").c_str()
                                  << "\n  -> available were:";
        for (auto const& [key, value] : m_modeIdMap) {
            qCWarning(DISMAN_WAYLAND).nospace() << value << ": " << key.c_str();
        }
    }
    return changed;
}

QString KWaylandOutput::modeName(const Wl::OutputDevice::Mode& m) const
{
    return QString::number(m.size.width()) + QLatin1Char('x') + QString::number(m.size.height())
        + QLatin1Char('@') + QString::number(qRound(m.refreshRate / 1000.0));
}

QString KWaylandOutput::name() const
{
    Q_ASSERT(m_device);
    return QStringLiteral("%1 %2").arg(m_device->manufacturer(), m_device->model());
}

QDebug operator<<(QDebug dbg, const KWaylandOutput* output)
{
    dbg << "KWaylandOutput(Id:" << output->id() << ", Name:"
        << QString(output->outputDevice()->manufacturer() + QLatin1Char(' ')
                   + output->outputDevice()->model())
        << ")";
    return dbg;
}
