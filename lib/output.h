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

#include <string>

namespace Disman
{

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

    Output();
    ~Output() override;

    OutputPtr clone() const;

    /**
     * Compares the data of this object with @param output.
     *
     * @return true if data is same otherwise false
     */
    bool compare(OutputPtr output) const;

    int id() const;
    void set_id(int id);

    /**
     * The name of the output uniquely identifies it and usually describes the connector in some way
     * together with a counter, for example DP-1, DP-2 and HDMI-A-1, HDMI-A-2.
     */
    std::string name() const;
    void set_name(std::string const& name);

    /**
     * The description of an output is provided by the backend and is meant to be displayed in
     * UI to identify the output and the display connected to it. It is pre-translated as it is the
     * responsibility of the backend to translate it to the user session locale.
     *
     * Examples are 'Foocorp 11" Display' or 'Virtual X11 output via :1'.
     */
    std::string description() const;
    void set_description(std::string const& description);

    /**
     * Returns an identifying hex encoded hash for this output.
     */
    std::string hash() const;

    /**
     * Set the output hash by providing a hashable input. The input will be hashed and converted
     * to hex internally.
     */
    void set_hash(std::string const& input);

    /**
     * Set the output hash by setting a hash directly.
     */
    void set_hash_raw(std::string const& hash);

    Type type() const;
    void setType(Type type);

    ModePtr mode(std::string const& id) const;
    ModePtr mode(QSize const& resolution, int refresh) const;

    ModeMap modes() const;
    void set_modes(const ModeMap& modes);

    /**
     * Sets the mode.
     *
     * @see commmanded_mode
     */
    void set_mode(ModePtr const& mode);
    bool set_resolution(QSize const& size);
    bool set_refresh_rate(int rate);

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
    int best_refresh_rate(QSize const& resolution) const;

    void set_preferred_modes(std::vector<std::string> const& modes);
    std::vector<std::string> const& preferred_modes() const;

    /**
     * Returns a mode that the hardware marked as preferred and that is the best one in the sense
     * of @ref best_mode().
     *
     * @see best_mode
     */
    ModePtr preferred_mode() const;

    Rotation rotation() const;
    void set_rotation(Rotation rotation);

    /**
     * Helper that returns true when output is not rotated or is rotated upside down.
     */
    bool horizontal() const;

    bool enabled() const;
    void set_enabled(bool enabled);

    bool adaptive_sync() const;
    void set_adaptive_sync(bool adapt);

    bool adaptive_sync_toggle_support() const;
    void set_adaptive_sync_toggle_support(bool support);

    /**
     * @brief Provides the source for an ongoing replication
     *
     * If the returned output id is non-null this output is a replica of the
     * returned output. If null is returned the output is no replica of any
     * other output.
     *
     * @return Replication source output id of this output
     */
    int replication_source() const;
    /**
     * @brief Set the replication source.
     * @param source
     */
    void set_replication_source(int source);

    /**
     * Returns the physical size of the screen in milimeters.
     *
     * @note Some broken GPUs or monitors return the size in centimeters instead
     * of millimeters. Disman at the moment is not sanitizing the values.
     */
    QSize physical_size() const;
    void set_physical_size(const QSize& size);

    /**
     * Returns if the output needs to be taken account for in the overall compositor/screen
     * space and if it should be depicted on its own in a graphical view for repositioning.
     *
     * @return true if the output is positionable in compositor/screen space.
     *
     * @since 5.17
     */
    bool positionable() const;

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
    void set_position(const QPointF& position);

    double scale() const;
    void set_scale(double scale);

    /**
     * @returns whether the mode should be changed to the new preferred mode
     * once it changes
     */
    bool follow_preferred_mode() const;

    /**
     * Set whether the preferred mode should be followed through @arg follow
     *
     * @since 5.15
     */
    void set_follow_preferred_mode(bool follow);

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

    struct GlobalData {
        QSize resolution;
        int refresh{0};
        bool adapt_sync{false};

        Rotation rotation;
        double scale;

        bool auto_resolution{true};
        bool auto_refresh_rate{true};
        bool auto_rotate{false};
        bool auto_rotate_only_in_tablet_mode{true};

        bool valid{false};
    };

    GlobalData global_data() const;
    void set_global_data(GlobalData data);

    std::string log() const;

Q_SIGNALS:
    /**
     * An update to the output was applied. Changes to its properties could have occured.
     */
    void updated();

private:
    Q_DISABLE_COPY(Output)

    class Private;
    Private* const d;

    Output(Private* dd);

    friend class Generator;
};

}

DISMAN_EXPORT QDebug operator<<(QDebug dbg, const Disman::OutputPtr& output);

Q_DECLARE_METATYPE(Disman::OutputMap)
Q_DECLARE_METATYPE(Disman::Output::Rotation)
Q_DECLARE_METATYPE(Disman::Output::Type)

#endif
