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
#pragma once

#include "output.h"
#include "waylandoutput.h"

#include <Wrapland/Client/output_device_v1.h>
#include <Wrapland/Client/registry.h>

#include <QRectF>
#include <string>

namespace Wrapland
{
namespace Client
{
class OutputConfigurationV1;
}
}

namespace Disman
{

class KwinftOutput : public WaylandOutput
{
    Q_OBJECT

public:
    explicit KwinftOutput(quint32 id, QObject* parent = nullptr);
    ~KwinftOutput() override = default;

    void updateDismanOutput(Disman::OutputPtr& output) override;

    QByteArray edid() const override;
    bool enabled() const override;
    QRectF geometry() const override;

    Wrapland::Client::OutputDeviceV1* outputDevice() const;
    void createOutputDevice(Wrapland::Client::Registry* registry, quint32 name, quint32 version);

    bool setWlConfig(Wrapland::Client::OutputConfigurationV1* wlConfig,
                     const Disman::OutputPtr& output);

private:
    void showOutput();
    QString hash() const;
    QString modeName(const Wrapland::Client::OutputDeviceV1::Mode& m) const;

    Wrapland::Client::OutputDeviceV1* m_device;
    Wrapland::Client::Registry* m_registry;

    // left-hand-side: Disman::Mode, right-hand-side: Wrapland's mode.id
    QMap<QString, int> m_modeIdMap;
};

}
