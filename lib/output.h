/*************************************************************************************
 *  Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *  Copyright (C) 2014 by Daniel Vrátil <dvratil@redhat.com>                         *
 *  Copyright © 2020 Roman Gilg <subdiff@gmail.com>                                  *
 *                                                                                   *
 *  This library is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU Lesser General Public                       *
 *  License as published by the Free Software Foundation; either                     *
 *  version 2.1 of the License, or (at your option) any later version.               *
 *                                                                                   *
 *  This library is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                *
 *  Lesser General Public License for more details.                                  *
 *                                                                                   *
 *  You should have received a copy of the GNU Lesser General Public                 *
 *  License along with this library; if not, write to the Free Software              *
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA       *
 *************************************************************************************/
#ifndef OUTPUT_CONFIG_H
#define OUTPUT_CONFIG_H

#include "disman_export.h"
#include "mode.h"
#include "types.h"

#include <QDebug>
#include <QMetaType>
#include <QObject>
#include <QPoint>
#include <QSize>
#include <QStringList>

namespace Disman
{

class Edid;

class DISMAN_EXPORT Output : public QObject
{
    Q_OBJECT

public:
    Q_ENUMS(Rotation)
    Q_ENUMS(Type)

    enum Type {
        Unknown,
        VGA,
        DVI,
        DVII,
        DVIA,
        DVID,
        HDMI,
        Panel,
        TV,
        TVComposite,
        TVSVideo,
        TVComponent,
        TVSCART,
        TVC4,
        DisplayPort,
    };

    enum Rotation {
        None = 1,
        Left = 2,
        Inverted = 4,
        Right = 8,
    };

    enum class Retention {
        Undefined = -1,
        Global = 0,
        Individual = 1,
    };
    Q_ENUM(Retention)

    explicit Output();
    ~Output() override;

    OutputPtr clone() const;

    int id() const;
    void setId(int id);

    /**
     * The name of the output uniquely identifies it and usually describes the connector in some way
     * together with a counter, for example DP-1, DP-2 and HDMI-A-1, HDMI-A-2.
     */
    std::string name() const;
    void set_name(std::string const& name);

    /**
     * Returns an identifying hex encoded hash for this output.
     *
     * The hash is calculated either via the edid hash or if no
     * edid is available by the output name, which is hashed as well.
     *
     * @return identifying hash of this output
     */
    QString hash() const;

    Type type() const;
    void setType(Type type);

    QString icon() const;
    void setIcon(const QString& icon);

    Q_INVOKABLE ModePtr mode(const QString& id) const;
    ModePtr mode(QSize const& resolution, double refresh_rate) const;

    ModeList modes() const;
    void setModes(const ModeList& modes);

    /**
     * Sets the mode.
     *
     * @see commmanded_mode
     */
    void set_mode(ModePtr const& mode);
    bool set_resolution(QSize const& size);
    bool set_refresh_rate(double rate);

    void set_to_preferred_mode();

    /**
     * Returns the mode determined by the set resolution and refresh rate. When received from the
     * backend this determines the current hardware mode independet of what auto_mode says.
     *
     * On the opposite if they are not the same the configuration is not in sync what indicates
     * a corruption of the output configuration.
     *
     * @see commanded_mode
     * @see set_resolution
     * @see set_refresh_rate
     */
    ModePtr commanded_mode() const;

    /**
     * Returns a best mode by first selecting the highest available resolution
     * and for that resolution the mode with the highest refresh rate.
     */
    ModePtr best_mode() const;

    /**
     * Similar to @ref best_mode, tries to return the best mode but considers the current auto
     * resolution and auto refresh rate settings.
     *
     * That means in a first step the highest available resolution is chosen if @ref auto_resolution
     * is true and otherwise the currently set resolution. In a second step the mode with the
     * highest refresh rate for this resolution will be returned if @ref auto_refresh_rate is true,
     * otherwise the mode with the currently set refresh rate. If such a mode does not exist null
     * is returned.
     */
    ModePtr auto_mode() const;

    QSize best_resolution() const;
    double best_refresh_rate(QSize const& resolution) const;

    void setPreferredModes(const QStringList& modes);
    QStringList preferredModes() const;

    /**
     * Returns a mode that the hardware marked as preferred and that is the best one in the sense
     * of @ref best_mode().
     *
     * @see best_mode
     */
    ModePtr preferred_mode() const;

    Rotation rotation() const;
    void setRotation(Rotation rotation);
    /**
     * A comfortable function that returns true when output is not rotated
     * or is rotated upside down.
     */
    Q_INVOKABLE inline bool isHorizontal() const
    {
        return ((rotation() == Output::None) || (rotation() == Output::Inverted));
    }

    bool isEnabled() const;
    void setEnabled(bool enabled);

    bool isPrimary() const;
    void setPrimary(bool primary);

    /**
     * @brief Provides the source for an ongoing replication
     *
     * If the returned output id is non-null this output is a replica of the
     * returned output. If null is returned the output is no replica of any
     * other output.
     *
     * @return Replication source output id of this output
     */
    int replicationSource() const;
    /**
     * @brief Set the replication source.
     * @param source
     */
    void setReplicationSource(int source);

    void setEdid(const QByteArray& rawData);

    /**
     * edid returns the output's EDID information if available.
     *
     * The output maintains ownership of the returned Edid, so the caller should not delete it.
     * Note that the edid is only valid as long as the output is alive.
     */
    Edid* edid() const;

    /**
     * Returns the physical size of the screen in milimeters.
     *
     * @note Some broken GPUs or monitors return the size in centimeters instead
     * of millimeters. Disman at the moment is not sanitizing the values.
     */
    QSize sizeMm() const;
    void setSizeMm(const QSize& size);

    /**
     * Returns if the output needs to be taken account for in the overall compositor/screen
     * space and if it should be depicted on its own in a graphical view for repositioning.
     *
     * @return true if the output is positionable in compositor/screen space.
     *
     * @since 5.17
     */
    bool isPositionable() const;

    /**
     * Returns a rectangle containing the currently set output position and
     * size.
     *
     * The geometry reflects current orientation (i.e. if current mode
     * is 1920x1080 and orientation is @p Disman::Output::Left, then the
     * size of the returned rectangle will be 1080x1920.
     *
     * The geometry also respects the set scale. But it is not influenced by a replication
     * source.
     *
     * This property contains the current settings stored in the particular
     * Output object, so it is updated even when user changes current mode
     * or orientation without applying the whole config.
     */
    QRectF geometry() const;

    /**
     * Force a certain geometry. This function is meant to be used by the backend. Values written
     * to it by frontend clients have no guarantee to be effective and might corrupt the config.
     *
     * @param geo to force
     */
    void force_geometry(QRectF const& geo);

    QPointF position() const;
    void setPosition(const QPointF& position);

    double scale() const;
    void setScale(double scale);

    /**
     * @returns whether the mode should be changed to the new preferred mode
     * once it changes
     */
    bool followPreferredMode() const;

    /**
     * Set whether the preferred mode should be followed through @arg follow
     *
     * @since 5.15
     */
    void setFollowPreferredMode(bool follow);

    bool auto_resolution() const;
    void set_auto_resolution(bool auto_res);

    bool auto_refresh_rate() const;
    void set_auto_refresh_rate(bool auto_rate);

    bool auto_rotate() const;
    void set_auto_rotate(bool auto_rot);

    bool auto_rotate_only_in_tablet_mode() const;
    void set_auto_rotate_only_in_tablet_mode(bool only);

    Retention retention() const;
    void set_retention(Retention retention);

    void apply(const OutputPtr& other);

Q_SIGNALS:
    /**
     * An update to the output was applied. Changes to its properties could have occured.
     */
    void updated();
    void isPrimaryChanged();

private:
    Q_DISABLE_COPY(Output)

    class Private;
    Private* const d;

    Output(Private* dd);
};

}

DISMAN_EXPORT QDebug operator<<(QDebug dbg, const Disman::OutputPtr& output);

Q_DECLARE_METATYPE(Disman::OutputList)
Q_DECLARE_METATYPE(Disman::Output::Rotation)
Q_DECLARE_METATYPE(Disman::Output::Type)

#endif
