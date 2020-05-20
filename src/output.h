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

#ifndef OUTPUT_CONFIG_H
#define OUTPUT_CONFIG_H

#include "mode.h"
#include "types.h"
#include "disman_export.h"

#include <QObject>
#include <QSize>
#include <QPoint>
#include <QMetaType>
#include <QStringList>
#include <QDebug>

namespace Disman {

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
            DisplayPort
        };

        enum Rotation {
            None = 1,
            Left = 2,
            Inverted = 4,
            Right = 8
        };

        explicit Output();
        ~Output() override;

        OutputPtr clone() const;

        int id() const;
        void setId(int id);

        QString name() const;
        void setName(const QString& name);

        /**
         * Returns an identifying hash for this output.
         *
         * The hash is calculated either via the edid hash or if no
         * edid is available by the output name.
         *
         * @return identifying hash of this output
         * @since 5.15
         * @deprecated
         * @see hashMd5
         */
        QString hash() const;

        /**
         * Returns an identifying hex encoded MD5-hash for this output.
         *
         * The hash is calculated either via the edid hash or if no
         * edid is available by the output name, which is hashed as well.
         *
         * @return identifying hash of this output
         * @since 5.17
         */
        QString hashMd5() const;

        Type type() const;
        void setType(Type type);

        QString icon() const;
        void setIcon(const QString& icon);

        Q_INVOKABLE ModePtr mode(const QString &id) const;
        ModeList modes() const;
        void setModes(const ModeList &modes);

        QString currentModeId() const;
        void setCurrentModeId(const QString& mode);
        Q_INVOKABLE ModePtr currentMode() const;

        void setPreferredModes(const QStringList &modes);
        QStringList preferredModes() const;
        /**
         * Returns the preferred mode with higher resolution and refresh
         */
        Q_INVOKABLE QString preferredModeId() const;
        /**
         * Returns Disman::Mode associated with preferredModeId()
         */
        Q_INVOKABLE ModePtr preferredMode() const;

        /**
         * Returns either current mode size or if not available preferred one or if also not
         * available the first one in the ModeList.
         *
         * @return mode size
         */
        QSize enforcedModeSize() const;

        Rotation rotation() const;
        void setRotation(Rotation rotation);
        /**
         * A comfortable function that returns true when output is not rotated
         * or is rotated upside down.
         */
        Q_INVOKABLE inline bool isHorizontal() const {
            return ((rotation() == Output::None) || (rotation() == Output::Inverted));
        }

        bool isConnected() const;
        void setConnected(bool connected);

        bool isEnabled() const;
        void setEnabled(bool enabled);

        bool isPrimary() const;
        void setPrimary(bool primary);

        /**
         * @brief Immutable clones because of hardware restrictions
         *
         * Clones are set symmetcally on all outputs. The list contains ids
         * for all other outputs being clones of this output.
         *
         * @return List of output ids being clones of each other.
         */
        QList<int> clones() const;
        /**
         * @brief Set the clones list.
         *
         * When this output is part of a configuration this call is followed by
         * similar calls on other outputs making the lists in all outputs
         * symmetric.
         * @param outputlist
         */
        void setClones(const QList<int> &outputlist);

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

        void setEdid(const QByteArray &rawData);

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
        void setSizeMm(const QSize &size);

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

        QPointF position() const;
        void setPosition(const QPointF &position);

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

        void apply(const OutputPtr &other);
    Q_SIGNALS:
        void outputChanged();
        void geometryChanged();
        void currentModeIdChanged();
        void rotationChanged();
        void isConnectedChanged();
        void isEnabledChanged();
        void isPrimaryChanged();
        void clonesChanged();
        void replicationSourceChanged();
        void followPreferredModeChanged(bool followPreferredMode);

        /** The mode list changed.
         *
         * This may happen when a mode is added or changed.
         *
         * @since 5.8.3
         */
        void modesChanged();

    private:
        Q_DISABLE_COPY(Output)

        class Private;
        Private * const d;

        Output(Private *dd);
};

} //Disman namespace

DISMAN_EXPORT QDebug operator<<(QDebug dbg, const Disman::OutputPtr &output);

Q_DECLARE_METATYPE(Disman::OutputList)
Q_DECLARE_METATYPE(Disman::Output::Rotation)
Q_DECLARE_METATYPE(Disman::Output::Type)

#endif //OUTPUT_H
