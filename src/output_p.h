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
#include "output.h"

#include <QScopedPointer>
#include <QStringList>

namespace Disman
{
class Edid;

class Q_DECL_HIDDEN Output::Private
{
public:
    Private();
    Private(const Private& other);

    QString biggestMode(const ModeList& modes) const;
    bool compareModeList(const ModeList& before, const ModeList& after);

    int id;
    QString name;
    Type type;
    QString icon;
    ModeList modeList;
    QList<int> clones;
    int replicationSource;
    QString currentModeId;
    QString preferredMode;
    QStringList preferredModes;
    QSize sizeMm;
    QPointF position;
    Rotation rotation;
    qreal scale;
    bool connected;
    bool enabled;
    bool primary;
    bool followPreferredMode = false;

    QScopedPointer<Edid> edid;
};

}
