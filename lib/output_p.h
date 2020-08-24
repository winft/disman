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

#include <QRectF>
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

    ModePtr mode(QSize const& resolution, double refresh_rate) const;

    template<typename M>
    ModePtr get_mode(M const& mode) const
    {
        return mode;
    }

    template<typename List>
    QSize best_resolution(const List& modes) const
    {
        QSize best_size{0, 0};

        for (auto _mode : modes) {
            auto mode = get_mode(_mode);
            auto mode_size = mode->size();
            if (best_size.width() * best_size.height() < mode_size.width() * mode_size.height()) {
                best_size = mode_size;
            }
        }
        return best_size;
    }

    template<typename List>
    double best_refresh_rate(const List& modes, QSize const& resolution) const
    {
        ModePtr best_mode;
        double best_refresh = 0.;

        for (auto _mode : modes) {
            auto mode = get_mode(_mode);
            if (resolution != mode->size()) {
                continue;
            }
            if (best_refresh < mode->refreshRate()) {
                best_mode = mode;
                best_refresh = mode->refreshRate();
            }
        }

        return best_refresh;
    }

    template<typename List>
    ModePtr best_mode(const List& modes) const
    {
        auto const resolution = best_resolution(modes);
        auto const refresh_rate = best_refresh_rate(modes, resolution);

        return mode(resolution, refresh_rate);
    }

    bool compareModeList(const ModeList& before, const ModeList& after);

    int id;
    QString name;
    Type type;
    QString icon;
    ModeList modeList;
    int replicationSource;

    QSize resolution;
    double refresh_rate{0};

    QString preferredMode;
    QStringList preferredModes;
    QSize sizeMm;
    QPointF position;
    QRectF enforced_geometry;
    Rotation rotation;
    qreal scale;
    bool enabled;
    bool primary;
    bool followPreferredMode = false;

    bool auto_resolution{false};
    bool auto_refresh_rate{false};
    bool auto_rotate{true};
    bool auto_rotate_only_in_tablet_mode{true};

    Retention retention{Retention::Undefined};

    QScopedPointer<Edid> edid;
};

template<>
ModePtr Output::Private::get_mode(const QString& modeId) const
{
    if (!modeList.contains(modeId)) {
        return ModePtr();
    }
    return modeList[modeId];
}

}
