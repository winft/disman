/*************************************************************************************
 *  Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *  Copyright (C) 2014 by Daniel Vrátil <dvratil@redhat.com>                         *
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
#ifndef DISMAN_CONFIG_H
#define DISMAN_CONFIG_H

#include "disman_export.h"
#include "screen.h"
#include "types.h"

#include <QHash>
#include <QMetaType>
#include <QObject>

namespace Disman
{

/**
 * Represents a (or the) screen configuration.
 *
 * This is the main class of Disman, with it you can use
 * the static methods current() to get the systems config and
 * setConfig() to apply a config to the system.
 *
 * Also, you can instantiate an empty Config, this is usually done
 * to create a config (with the objective of setting it) from scratch
 * and for example unserialize a saved config to it.
 *
 */
class DISMAN_EXPORT Config : public QObject
{
    Q_OBJECT

public:
    enum class Cause {
        unknown,
        file,
        generated,
        interactive,
    };

    enum class ValidityFlag {
        None = 0x0,
        RequireAtLeastOneEnabledScreen = 0x1,
    };
    Q_DECLARE_FLAGS(ValidityFlags, ValidityFlag)

    /** This indicates which features the used backend supports.
     *
     * @see supported_features
     * @since 5.7
     */
    enum class Feature {
        None = 0,           ///< None of the mentioned features are supported.
        PrimaryDisplay = 1, ///< The backend knows about the concept of a primary display, this is
                            ///< mostly limited to X11.
        Writable = 1 << 1,  ///< The backend supports setting the config, it's not read-only.
        PerOutputScaling = 1 << 2,  ///< The backend supports scaling each output individually.
        OutputReplication = 1 << 3, ///< The backend supports replication of outputs.
        AutoRotation = 1 << 4,      ///< The backend supports automatic rotation of outputs.
        TabletMode = 1 << 5, ///< The backend supports querying if a device is in tablet mode.
    };
    Q_DECLARE_FLAGS(Features, Feature)

    /**
     * Validates that a config can be applied in the current system
     *
     * Each system has different constrains, this method will test
     * the given config with those constrains to see if it
     * can be applied.
     *
     * @arg config to be checked
     * @flags enable additional optional checks
     * @return true if the configuration can be applied, false if not.
     * @since 5.3.0
     */
    static bool can_be_applied(const ConfigPtr& config, ValidityFlags flags);

    /**
     * Validates that a config can be applied in the current system
     *
     * Each system has different constrains, this method will test
     * the given config with those constrains to see if it
     * can be applied.
     *
     * @arg config to be checked
     * @return true if the configuration can be applied, false if not.
     */
    static bool can_be_applied(const ConfigPtr& config);

    /**
     * Instantiate an empty config
     *
     * Usually you do not want to use this constructor since there are some
     * values that make no sense to set (for example you want the Screen of
     * the current systme).
     *
     * So usually what you do is call current() and then modify
     * whatever you need.
     */
    Config();
    explicit Config(Cause cause);
    ~Config() override;

    /**
     * Duplicates the config
     *
     * @return a new Config instance with same property values
     */
    ConfigPtr clone() const;

    /**
     * Compares the data of this object with @param config.
     *
     * @return true if data is same otherwise false
     */
    bool compare(ConfigPtr config) const;

    /**
     * Returns an identifying hash for this config in regards to its
     * connected outputs.
     *
     * The hash is calculated with a sorted combination of all
     * connected output hashes.
     *
     * @return sorted hash combination of all connected outputs
     * @since 5.15
     */
    QString hash() const;

    Cause cause() const;
    void set_cause(Cause cause);

    ScreenPtr screen() const;
    void setScreen(const ScreenPtr& screen);

    OutputPtr output(int outputId) const;
    OutputMap outputs() const;

    OutputPtr primary_output() const;
    void set_primary_output(const OutputPtr& output);

    void add_output(const OutputPtr& output);
    void remove_output(int outputId);
    void set_outputs(OutputMap const& outputs);

    bool valid() const;
    void set_valid(bool valid);

    void apply(const ConfigPtr& other);

    /** Indicates features supported by the backend. This exists to allow the user
     * to find out which of the features offered by disman are actually supported
     * by the backend. Not all backends are writable (QScreen, for example is
     * read-only, only XRandR, but not KWayland support the primary display, etc.).
     *
     * @return Flags for features that are supported for this config, determined by
     * the backend.
     * @see set_supported_features
     * @since 5.7
     */
    Features supported_features() const;

    /** Sets the features supported by this backend. This should not be called by the
     * user, but by the backend.
     *
     * @see supported_features
     * @since 5.7
     */
    void set_supported_features(const Features& features);

    /**
     * Indicates that the device supports switching between a default and a tablet mode. This is
     * common for convertibles.
     *
     * @return true when tablet mode is available, otherwise false
     * @see set_tablet_mode_available
     * @since 5.18
     */
    bool tablet_mode_available() const;

    /** Sets if the device supports a tablet mode. This should not be called by the
     * user, but by the backend.
     *
     * @see tablet_mode_available
     * @since 5.18
     */
    void set_tablet_mode_available(bool available);

    /**
     * Indicates that the device is currently in tablet mode.
     *
     * @return true when in tablet mode, otherwise false
     * @see set_tablet_mode_engaged
     * @since 5.18
     */
    bool tablet_mode_engaged() const;

    /**
     * Sets if the device is currently in tablet mode. This should not be called by the
     * user, but by the backend.
     *
     * @see tabletModeEngaged
     * @since 5.18
     */
    void set_tablet_mode_engaged(bool engaged);

    /**
     * Returns the Output @param output replicates if @param output is a replica, otherwise null.
     *
     * @param output to find replication source for
     * @return replication source or null
     */
    OutputPtr replication_source(OutputPtr const& output);

Q_SIGNALS:
    void output_added(const Disman::OutputPtr& output);
    void output_removed(int outputId);
    void primary_output_changed(const Disman::OutputPtr& output);

private:
    Q_DISABLE_COPY(Config)

    class Private;
    Private* const d;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Disman::Config::Features)

DISMAN_EXPORT QDebug operator<<(QDebug dbg, const Disman::ConfigPtr& config);

#endif
