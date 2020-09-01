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

#include <Wrapland/Client/registry.h>
#include <Wrapland/Client/wlr_output_manager_v1.h>

#include <QRectF>

namespace Wrapland
{
namespace Client
{
class WlrOutputConfigurationV1;
class WlrOutputHeadV1;
class WlrOutputModeV1;
}
}

namespace Disman
{
class WlrootsInterface;

class WlrootsOutput : public WaylandOutput
{
    Q_OBJECT

public:
    explicit WlrootsOutput(quint32 id,
                           Wrapland::Client::WlrOutputHeadV1* head,
                           WlrootsInterface* parent);
    ~WlrootsOutput() override = default;

    void updateDismanOutput(Disman::OutputPtr& output) override;

    QByteArray edid() const override;
    bool enabled() const override;
    QRectF geometry() const override;

    Wrapland::Client::WlrOutputHeadV1* outputHead() const;

    bool setWlConfig(Wrapland::Client::WlrOutputConfigurationV1* wlConfig,
                     const Disman::OutputPtr& output);

private:
    void showOutput();

    Wrapland::Client::WlrOutputHeadV1* m_head;
    Wrapland::Client::Registry* m_registry;

    // left-hand-side: Disman::Mode, right-hand-side: Wrapland's WlrOutputModeV1
    QMap<QString, Wrapland::Client::WlrOutputModeV1*> m_modeIdMap;
};

}
