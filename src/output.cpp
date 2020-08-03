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
#include "output_p.h"

#include "abstractbackend.h"
#include "backendmanager_p.h"
#include "disman_debug.h"
#include "edid.h"
#include "mode.h"

#include <QCryptographicHash>
#include <QRect>
#include <QScopedPointer>
#include <QStringList>

namespace Disman
{

Output::Private::Private()
    : id(0)
    , type(Unknown)
    , replicationSource(0)
    , rotation(None)
    , scale(1.0)
    , enabled(false)
    , primary(false)
    , edid(nullptr)
{
}

Output::Private::Private(const Private& other)
    : id(other.id)
    , name(other.name)
    , type(other.type)
    , icon(other.icon)
    , replicationSource(other.replicationSource)
    , currentModeId(other.currentModeId)
    , preferredMode(other.preferredMode)
    , preferredModes(other.preferredModes)
    , sizeMm(other.sizeMm)
    , position(other.position)
    , rotation(other.rotation)
    , scale(other.scale)
    , enabled(other.enabled)
    , primary(other.primary)
    , followPreferredMode(other.followPreferredMode)
{
    Q_FOREACH (const ModePtr& otherMode, other.modeList) {
        modeList.insert(otherMode->id(), otherMode->clone());
    }
    if (other.edid) {
        edid.reset(other.edid->clone());
    }
}

ModePtr Output::Private::mode(QSize const& resolution, double refresh_rate) const
{
    for (auto mode : modeList) {
        if (resolution == mode->size() && refresh_rate == mode->refreshRate()) {
            return mode;
        }
    }
    return ModePtr();
}

bool Output::Private::compareModeList(const ModeList& before, const ModeList& after)
{
    if (before.count() != after.count()) {
        return false;
    }

    for (auto itb = before.constBegin(); itb != before.constEnd(); ++itb) {
        auto ita = after.constFind(itb.key());
        if (ita == after.constEnd()) {
            return false;
        }
        const auto& mb = itb.value();
        const auto& ma = ita.value();
        if (mb->id() != ma->id()) {
            return false;
        }
        if (mb->size() != ma->size()) {
            return false;
        }
        if (!qFuzzyCompare(mb->refreshRate(), ma->refreshRate())) {
            return false;
        }
        if (mb->name() != ma->name()) {
            return false;
        }
    }
    // They're the same
    return true;
}

Output::Output()
    : QObject(nullptr)
    , d(new Private())
{
}

Output::Output(Output::Private* dd)
    : QObject()
    , d(dd)
{
}

Output::~Output()
{
    delete d;
}

OutputPtr Output::clone() const
{
    return OutputPtr(new Output(new Private(*d)));
}

int Output::id() const
{
    return d->id;
}

void Output::setId(int id)
{
    d->id = id;
}

QString Output::name() const
{
    return d->name;
}

void Output::setName(const QString& name)
{
    d->name = name;
}

// TODO KF6: remove this deprecated method
QString Output::hash() const
{
    if (edid() && edid()->isValid()) {
        return edid()->hash();
    }
    return name();
}

QString Output::hashMd5() const
{
    if (edid() && edid()->isValid()) {
        return edid()->hash();
    }
    const auto hash = QCryptographicHash::hash(name().toLatin1(), QCryptographicHash::Md5);
    return QString::fromLatin1(hash.toHex());
}

Output::Type Output::type() const
{
    return d->type;
}

void Output::setType(Type type)
{
    d->type = type;
}

QString Output::icon() const
{
    return d->icon;
}

void Output::setIcon(const QString& icon)
{
    d->icon = icon;
}

ModePtr Output::mode(const QString& id) const
{
    if (!d->modeList.contains(id)) {
        return ModePtr();
    }

    return d->modeList[id];
}

ModeList Output::modes() const
{
    return d->modeList;
}

void Output::setModes(const ModeList& modes)
{
    d->modeList = modes;
}

QString Output::currentModeId() const
{
    return d->currentModeId;
}

void Output::setCurrentModeId(const QString& modeId)
{
    d->currentModeId = modeId;
}

ModePtr Output::currentMode() const
{
    return d->modeList.value(d->currentModeId);
}

void Output::setPreferredModes(const QStringList& modes)
{
    d->preferredMode = QString();
    d->preferredModes = modes;
}

QStringList Output::preferredModes() const
{
    return d->preferredModes;
}

QString Output::preferredModeId() const
{
    if (!d->preferredMode.isEmpty()) {
        return d->preferredMode;
    }
    if (d->preferredModes.isEmpty()) {
        auto best_mode = d->best_mode(modes());
        return best_mode->id();
    }

    auto best = d->best_mode(d->preferredModes);
    Q_ASSERT_X(best, "preferredModeId", "biggest mode must exist");

    d->preferredMode = best->id();
    return d->preferredMode;
}

ModePtr Output::preferredMode() const
{
    return d->modeList.value(preferredModeId());
}

void Output::setPosition(const QPointF& position)
{
    d->position = position;
}

// TODO KF6: make the Rotation enum an enum class and align values with Wayland transformation
// property
Output::Rotation Output::rotation() const
{
    return d->rotation;
}

void Output::setRotation(Output::Rotation rotation)
{
    d->rotation = rotation;
}

double Output::scale() const
{
    return d->scale;
}

void Output::setScale(double scale)
{
    d->scale = scale;
}

QRectF Output::geometry() const
{
    auto geo = QRectF(d->position, QSizeF());

    auto size = enforcedModeSize();
    if (!size.isValid()) {
        return geo;
    }
    if (!isHorizontal()) {
        size = size.transposed();
    }

    geo.setSize(size / d->scale);
    return geo;
}

QPointF Output::position() const
{
    return d->position;
}

bool Output::isEnabled() const
{
    return d->enabled;
}

void Output::setEnabled(bool enabled)
{
    d->enabled = enabled;
}

bool Output::isPrimary() const
{
    return d->primary;
}

void Output::setPrimary(bool primary)
{
    if (d->primary == primary) {
        return;
    }

    d->primary = primary;

    Q_EMIT isPrimaryChanged();
}

int Output::replicationSource() const
{
    return d->replicationSource;
}

void Output::setReplicationSource(int source)
{
    d->replicationSource = source;
}

void Output::setEdid(const QByteArray& rawData)
{
    Q_ASSERT(d->edid.isNull());
    d->edid.reset(new Edid(rawData));
}

Edid* Output::edid() const
{
    return d->edid.data();
}

QSize Output::sizeMm() const
{
    return d->sizeMm;
}

void Output::setSizeMm(const QSize& size)
{
    d->sizeMm = size;
}

bool Disman::Output::followPreferredMode() const
{
    return d->followPreferredMode;
}

void Disman::Output::setFollowPreferredMode(bool follow)
{
    d->followPreferredMode = follow;
}

bool Output::isPositionable() const
{
    return isEnabled() && !replicationSource();
}

QSize Output::enforcedModeSize() const
{
    if (const auto mode = currentMode()) {
        return mode->size();
    } else if (const auto mode = preferredMode()) {
        return mode->size();
    } else if (d->modeList.count() > 0) {
        return d->modeList.first()->size();
    }
    return QSize();
}

void Output::apply(const OutputPtr& other)
{
    typedef void (Disman::Output::*ChangeSignal)();
    QList<ChangeSignal> changes;

    // We block all signals, and emit them only after we have set up everything
    // This is necessary in order to prevent clients from accessing inconsistent
    // outputs from intermediate change signals
    const bool keepBlocked = signalsBlocked();
    blockSignals(true);

    setName(other->d->name);
    setType(other->d->type);
    setIcon(other->d->icon);
    setPosition(other->geometry().topLeft());
    setRotation(other->d->rotation);
    setScale(other->d->scale);
    setCurrentModeId(other->d->currentModeId);
    setEnabled(other->d->enabled);

    if (d->primary != other->d->primary) {
        changes << &Output::isPrimaryChanged;
        setPrimary(other->d->primary);
    }

    setReplicationSource(other->d->replicationSource);

    setPreferredModes(other->d->preferredModes);
    ModeList modes;
    Q_FOREACH (const ModePtr& otherMode, other->modes()) {
        modes.insert(otherMode->id(), otherMode->clone());
    }
    setModes(modes);

    // Non-notifyable changes
    if (other->d->edid) {
        d->edid.reset(other->d->edid->clone());
    }

    blockSignals(keepBlocked);

    while (!changes.isEmpty()) {
        const ChangeSignal& sig = changes.first();
        Q_EMIT(this->*sig)();
        changes.removeAll(sig);
    }

    Q_EMIT updated();
}

}

QDebug operator<<(QDebug dbg, const Disman::OutputPtr& output)
{
    if (output) {
        dbg << "Disman::Output(" << output->id() << " " << output->name()
            << (output->isEnabled() ? "enabled" : "disabled")
            << (output->isPrimary() ? "primary" : "") << "geometry:" << output->geometry()
            << "scale:" << output->scale() << "modeId:" << output->currentModeId()
            << "followPreferredMode:" << output->followPreferredMode() << ")";
    } else {
        dbg << "Disman::Output(NULL)";
    }
    return dbg;
}
