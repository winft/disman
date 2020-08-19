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

#include <disman/edid.h>
#include <disman/mode.h>

#include "kwinft_logging.h"

#include <Wrapland/Client/output_configuration_v1.h>
#include <Wrapland/Client/output_device_v1.h>

using namespace Disman;
namespace Wl = Wrapland::Client;

const QMap<Wl::OutputDeviceV1::Transform, Output::Rotation> s_rotationMap
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
    auto it = s_rotationMap.constFind(transform);
    return it.value();
}

Wl::OutputDeviceV1::Transform toWraplandTransform(const Output::Rotation rotation)
{
    return s_rotationMap.key(rotation);
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
    return m_device->edid();
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
    output->setEnabled(m_device->enabled() == Wl::OutputDeviceV1::Enablement::Enabled);
    output->setPrimary(true); // FIXME: wayland doesn't have the concept of a primary display
    output->setName(name());
    output->setSizeMm(m_device->physicalSize());
    output->setPosition(m_device->geometry().topLeft());
    output->setRotation(s_rotationMap[m_device->transform()]);

    ModeList modeList;

    ModePtr current_mode;
    QStringList preferredModeIds;
    m_modeIdMap.clear();

    for (const Wl::OutputDeviceV1::Mode& wlMode : m_device->modes()) {
        ModePtr mode(new Mode());
        const QString name = modeName(wlMode);

        QString modeId = QString::number(wlMode.id);
        if (modeId.isEmpty()) {
            qCWarning(DISMAN_WAYLAND)
                << "Could not create mode id from" << wlMode.id << ", using" << name << "instead.";
            modeId = name;
        }

        if (m_modeIdMap.contains(modeId)) {
            qCWarning(DISMAN_WAYLAND) << "Mode id already in use:" << modeId;
        }
        mode->setId(modeId);

        // Wrapland gives the refresh rate as int in mHz
        mode->setRefreshRate(wlMode.refreshRate / 1000.0);
        mode->setSize(wlMode.size);
        mode->setName(name);

        if (wlMode == m_device->currentMode()) {
            current_mode = mode;
        }
        if (wlMode.preferred) {
            preferredModeIds << modeId;
        }

        // Update the Disman => Wrapland mode id translation map
        m_modeIdMap.insert(modeId, wlMode.id);
        // Add to the modelist which gets set on the output
        modeList[modeId] = mode;
    }

    output->setPreferredModes(preferredModeIds);
    output->setModes(modeList);

    if (current_mode.isNull()) {
        qCWarning(DISMAN_WAYLAND) << "Could not find the current mode in:";
        for (auto const mode : modeList) {
            qCWarning(DISMAN_WAYLAND) << "  " << mode;
        }
    } else {
        output->set_mode(current_mode);
        output->set_resolution(current_mode->size());
        auto success = output->set_refresh_rate(current_mode->refreshRate());
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
    if ((m_device->enabled() == Wl::OutputDeviceV1::Enablement::Enabled) != output->isEnabled()) {
        changed = true;
        const auto enablement = output->isEnabled() ? Wl::OutputDeviceV1::Enablement::Enabled
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
    if (auto mode = output->auto_mode(); mode && m_modeIdMap.contains(mode->id())) {
        const int newModeId = m_modeIdMap.value(mode->id(), -1);
        if (newModeId != m_device->currentMode().id) {
            changed = true;
            wlConfig->setMode(m_device, newModeId);
        }
    } else {
        qCWarning(DISMAN_WAYLAND) << "Invalid Disman mode:"
                                  << (mode ? mode->id() : QStringLiteral("null"))
                                  << "\n  -> available were:" << m_modeIdMap;
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

QString KwinftOutput::name() const
{
    Q_ASSERT(m_device);
    return QStringLiteral("%1 %2").arg(m_device->manufacturer(), m_device->model());
}

QDebug operator<<(QDebug dbg, const KwinftOutput* output)
{
    dbg << "KwinftOutput(Id:" << output->id() << ", Name:"
        << QString(output->outputDevice()->manufacturer() + QLatin1Char(' ')
                   + output->outputDevice()->model())
        << ")";
    return dbg;
}
