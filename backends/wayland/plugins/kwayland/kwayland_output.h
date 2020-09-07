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
#pragma once

#include "waylandoutput.h"

#include "output.h"

#include <KWayland/Client/outputdevice.h>
#include <KWayland/Client/registry.h>

#include <QRectF>

namespace KWayland
{
namespace Client
{
class OutputConfiguration;
}
}

namespace Disman
{

class KWaylandOutput : public WaylandOutput
{
    Q_OBJECT

public:
    explicit KWaylandOutput(quint32 id, QObject* parent = nullptr);
    ~KWaylandOutput() override = default;

    void updateDismanOutput(Disman::OutputPtr& output) override;

    QString name() const;
    bool enabled() const override;
    QRectF geometry() const override;

    KWayland::Client::OutputDevice* outputDevice() const;
    void createOutputDevice(KWayland::Client::Registry* registry, quint32 name, quint32 version);

    bool setWlConfig(KWayland::Client::OutputConfiguration* wlConfig,
                     const Disman::OutputPtr& output);

private:
    void showOutput();
    QString modeName(const KWayland::Client::OutputDevice::Mode& m) const;

    KWayland::Client::OutputDevice* m_device;
    KWayland::Client::Registry* m_registry;

    // left-hand-side: Disman::Mode, right-hand-side: KWayland's mode.id
    std::map<std::string, int> m_modeIdMap;
};

}
