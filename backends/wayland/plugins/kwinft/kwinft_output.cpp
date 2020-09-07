/*************************************************************************
Copyright © 2020 Roman Gilg <subdiff@gmail.com>

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
#include "kwinft_output.h"

#include <mode.h>

#include "kwinft_logging.h"

#include <Wrapland/Client/output_configuration_v1.h>
#include <Wrapland/Client/output_device_v1.h>

#include <cassert>

using namespace Disman;
namespace Wl = Wrapland::Client;

const std::map<Wl::OutputDeviceV1::Transform, Output::Rotation> s_rotationMap
    = {{Wl::OutputDeviceV1::Transform::Normal, Output::None},
       {Wl::OutputDeviceV1::Transform::Rotated90, Output::Right},
       {Wl::OutputDeviceV1::Transform::Rotated180, Output::Inverted},
       {Wl::OutputDeviceV1::Transform::Rotated270, Output::Left},
       {Wl::OutputDeviceV1::Transform::Flipped, Output::None},
       {Wl::OutputDeviceV1::Transform::Flipped90, Output::Right},
       {Wl::OutputDeviceV1::Transform::Flipped180, Output::Inverted},
       {Wl::OutputDeviceV1::Transform::Flipped270, Output::Left}};

Output::Rotation toDismanRotation(const Wl::OutputDeviceV1::Transform transform)
{
    auto it = s_rotationMap.find(transform);
    assert(it != s_rotationMap.end());
    return it->second;
}

Wl::OutputDeviceV1::Transform toWraplandTransform(const Output::Rotation rotation)
{
    for (auto const& [key, val] : s_rotationMap) {
        if (val == rotation) {
            return key;
        }
    }
    assert(false);
}

KwinftOutput::KwinftOutput(quint32 id, QObject* parent)
    : WaylandOutput(id, parent)
    , m_device(nullptr)
{
}

bool KwinftOutput::enabled() const
{
    return m_device != nullptr;
}

QByteArray KwinftOutput::edid() const
{
    return QByteArray();
}

QRectF KwinftOutput::geometry() const
{
    return m_device->geometry();
}

Wrapland::Client::OutputDeviceV1* KwinftOutput::outputDevice() const
{
    return m_device;
}

void KwinftOutput::createOutputDevice(Wl::Registry* registry, quint32 name, quint32 version)
{
    Q_ASSERT(!m_device);
    m_device = registry->createOutputDeviceV1(name, version);

    connect(m_device, &Wl::OutputDeviceV1::removed, this, &KwinftOutput::removed);
    connect(m_device, &Wl::OutputDeviceV1::done, this, [this]() {
        disconnect(m_device, &Wl::OutputDeviceV1::done, this, nullptr);
        connect(m_device, &Wl::OutputDeviceV1::changed, this, &KwinftOutput::changed);
        Q_EMIT dataReceived();
    });
}

void KwinftOutput::updateDismanOutput(OutputPtr& output)
{
    // Initialize primary output
    output->set_enabled(m_device->enabled() == Wl::OutputDeviceV1::Enablement::Enabled);
    output->set_name(m_device->name().toStdString());
    output->set_description(m_device->description().toStdString());
    output->set_hash(hash().toStdString());
    output->set_physical_size(m_device->physicalSize());
    output->set_position(m_device->geometry().topLeft());
    output->set_rotation(s_rotationMap.at(m_device->transform()));

    ModeMap modeList;

    ModePtr current_mode;
    std::vector<std::string> preferredModeIds;
    m_modeIdMap.clear();

    for (const Wl::OutputDeviceV1::Mode& wlMode : m_device->modes()) {
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

        // Wrapland gives the refresh rate as int in mHz
        mode->set_refresh(wlMode.refreshRate / 1000.0);
        mode->set_size(wlMode.size);
        mode->set_name(name);

        if (wlMode == m_device->currentMode()) {
            current_mode = mode;
        }
        if (wlMode.preferred) {
            preferredModeIds.push_back(modeId);
        }

        // Update the Disman => Wrapland mode id translation map
        m_modeIdMap.insert({modeId, wlMode.id});
        // Add to the modelist which gets set on the output
        modeList[modeId] = mode;
    }

    output->set_preferred_modes(preferredModeIds);
    output->set_modes(modeList);

    if (!current_mode) {
        qCWarning(DISMAN_WAYLAND) << "Could not find the current mode in:";
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

    output->setType(guessType(m_device->model(), m_device->model()));
}

bool KwinftOutput::setWlConfig(Wl::OutputConfigurationV1* wlConfig, const Disman::OutputPtr& output)
{
    bool changed = false;

    // enabled?
    if ((m_device->enabled() == Wl::OutputDeviceV1::Enablement::Enabled) != output->enabled()) {
        changed = true;
        const auto enablement = output->enabled() ? Wl::OutputDeviceV1::Enablement::Enabled
                                                  : Wl::OutputDeviceV1::Enablement::Disabled;
        wlConfig->setEnabled(m_device, enablement);
    }

    // position
    if (m_device->geometry().topLeft() != output->position()) {
        changed = true;
        wlConfig->setGeometry(m_device, QRectF(output->position(), m_device->geometry().size()));
    }

    // rotation
    if (toDismanRotation(m_device->transform()) != output->rotation()) {
        changed = true;
        wlConfig->setTransform(m_device, toWraplandTransform(output->rotation()));
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

    // logical size
    if (m_device->geometry().size() != output->geometry().size()) {
        QSizeF size = output->geometry().size();
        changed = true;
        wlConfig->setGeometry(m_device, QRectF(output->position(), size));
    }
    return changed;
}

QString KwinftOutput::modeName(const Wl::OutputDeviceV1::Mode& m) const
{
    return QString::number(m.size.width()) + QLatin1Char('x') + QString::number(m.size.height())
        + QLatin1Char('@') + QString::number(qRound(m.refreshRate / 1000.0));
}

QString KwinftOutput::hash() const
{
    assert(m_device);
    return QStringLiteral("%1:%2:%3:%4")
        .arg(m_device->make(), m_device->model(), m_device->serialNumber(), m_device->name());
}

QDebug operator<<(QDebug dbg, const KwinftOutput* output)
{
    dbg << "KwinftOutput(Id:" << output->id() << ", Name:"
        << QString(output->outputDevice()->make() + QLatin1Char(' ')
                   + output->outputDevice()->model())
        << ")";
    return dbg;
}
