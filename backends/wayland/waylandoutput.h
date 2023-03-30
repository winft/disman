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

#include <output.h>

#include <QString>
#include <Wrapland/Client/registry.h>
#include <Wrapland/Client/wlr_output_manager_v1.h>

namespace Disman
{

class WaylandInterface;

class WaylandOutput : public QObject
{
    Q_OBJECT
public:
    WaylandOutput(uint32_t id, Wrapland::Client::WlrOutputHeadV1& head, WaylandInterface* iface);
    ~WaylandOutput() override = default;

    Disman::OutputPtr toDismanOutput();
    void updateDismanOutput(Disman::OutputPtr& output);

    QRectF geometry() const;

    bool setWlConfig(Wrapland::Client::WlrOutputConfigurationV1* wlConfig,
                     const Disman::OutputPtr& output);

    Disman::Output::Type guessType(const QString& type, const QString& name) const;

    uint32_t id;
    Wrapland::Client::WlrOutputHeadV1& head;

Q_SIGNALS:
    void dataReceived();
    void changed();
    void removed();

private:
    void showOutput();
    QString hash() const;

    Wrapland::Client::Registry* m_registry;

    // left-hand-side: Disman::Mode, right-hand-side: Wrapland's WlrOutputModeV1
    std::map<std::string, Wrapland::Client::WlrOutputModeV1*> m_modeIdMap;
};

}
