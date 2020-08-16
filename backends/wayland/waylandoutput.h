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

#include <disman/output.h>

#include "disman_wayland_export.h"

#include <QString>

namespace Disman
{
class WaylandInterface;

class DISMAN_WAYLAND_EXPORT WaylandOutput : public QObject
{
    Q_OBJECT

public:
    explicit WaylandOutput(quint32 id, QObject* parent = nullptr);
    ~WaylandOutput() override = default;

    Disman::OutputPtr toDismanOutput();
    virtual void updateDismanOutput(Disman::OutputPtr& output) = 0;

    virtual quint32 id() const;
    virtual QByteArray edid() const = 0;
    virtual bool enabled() const = 0;
    virtual QRectF geometry() const = 0;

    Disman::Output::Type guessType(const QString& type, const QString& name) const;

Q_SIGNALS:
    void dataReceived();
    void changed();
    void removed();

private:
    quint32 m_id;

    // left-hand-side: Disman::Mode, right-hand-side: Compositor's mode id
    QMap<QString, int> m_modeIdMap;
};

}
