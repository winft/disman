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
#include "output_p.h"

#include "disman_debug.h"
#include "mode.h"

#include <QCryptographicHash>
#include <QRect>

#include <sstream>

namespace Disman
{

Output::Private::Private()
    : id(0)
    , type(Unknown)
    , replication_source(0)
    , rotation(None)
    , scale(1.0)
    , enabled(false)
{
}

Output::Private::Private(const Private& other)
    : id(other.id)
    , name(other.name)
    , description(other.description)
    , hash(other.hash)
    , type(other.type)
    , replication_source(other.replication_source)
    , resolution(other.resolution)
    , refresh_rate(other.refresh_rate)
    , preferredMode(other.preferredMode)
    , preferred_modes(other.preferred_modes)
    , physical_size(other.physical_size)
    , position(other.position)
    , rotation(other.rotation)
    , scale(other.scale)
    , enabled(other.enabled)
    , follow_preferred_mode(other.follow_preferred_mode)
    , auto_resolution{other.auto_resolution}
    , auto_refresh_rate{other.auto_refresh_rate}
    , auto_rotate{other.auto_rotate}
    , auto_rotate_only_in_tablet_mode{other.auto_rotate_only_in_tablet_mode}
    , retention{other.retention}
    , global{other.global}
{
    for (auto const& [key, otherMode] : other.modeList) {
        modeList.insert({key, otherMode->clone()});
    }
}

ModePtr Output::Private::mode(QSize const& resolution, int refresh) const
{
    for (auto const& [key, mode] : modeList) {
        if (resolution == mode->size() && refresh == mode->refresh()) {
            return mode;
        }
    }
    return ModePtr();
}

bool Output::Private::compareModeMap(const ModeMap& before, const ModeMap& after)
{
    if (before.size() != after.size()) {
        return false;
    }

    for (auto const& [before_key, before_val] : before) {
        auto ita = after.find(before_key);
        if (ita == after.end()) {
            return false;
        }

        const auto& mb = before_val;
        const auto& ma = ita->second;

        if (mb->id() != ma->id()) {
            return false;
        }
        if (mb->size() != ma->size()) {
            return false;
        }
        if (mb->refresh() != ma->refresh()) {
            return false;
        }
        if (mb->name() != ma->name()) {
            return false;
        }
    }
    // They're the same
    return true;
}

void Output::Private::apply_global()
{
    if (!global.valid) {
        return;
    }
    if (retention == Output::Retention::Individual) {
        return;
    }

    resolution = global.resolution;
    refresh_rate = global.refresh;
    rotation = global.rotation;
    scale = global.scale;
    auto_resolution = global.auto_resolution;
    auto_refresh_rate = global.auto_refresh_rate;
    auto_rotate = global.auto_rotate;
    auto_rotate_only_in_tablet_mode = global.auto_rotate_only_in_tablet_mode;
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

bool Output::compare(OutputPtr output) const
{
    if (!output) {
        return false;
    }

    auto const simple_data_compare = d->id == output->d->id && d->name == output->d->name
        && d->description == output->d->description && d->hash == output->d->hash
        && d->type == output->d->type && d->replication_source == output->d->replication_source
        && d->resolution == output->d->resolution && d->refresh_rate == output->d->refresh_rate
        && d->preferredMode == output->d->preferredMode
        && d->preferred_modes == output->d->preferred_modes
        && d->physical_size == output->d->physical_size && d->position == output->d->position
        && d->enforced_geometry == output->d->enforced_geometry
        && d->rotation == output->d->rotation && d->scale == output->d->scale
        && d->enabled == output->d->enabled
        && d->follow_preferred_mode == output->d->follow_preferred_mode
        && d->auto_resolution == output->d->auto_resolution
        && d->auto_refresh_rate == output->d->auto_refresh_rate
        && d->auto_rotate == output->d->auto_rotate
        && d->auto_rotate_only_in_tablet_mode == output->d->auto_rotate_only_in_tablet_mode
        && d->retention == output->d->retention;

    auto const& own_global = d->global;
    auto const& other_global = output->d->global;

    auto const global_data_compare = own_global.resolution == other_global.resolution
        && own_global.refresh == other_global.refresh
        && own_global.rotation == other_global.rotation && own_global.scale == other_global.scale
        && own_global.auto_resolution == other_global.auto_resolution
        && own_global.auto_refresh_rate == other_global.auto_refresh_rate
        && own_global.auto_rotate == other_global.auto_rotate
        && own_global.auto_rotate_only_in_tablet_mode
            == other_global.auto_rotate_only_in_tablet_mode
        && own_global.valid == other_global.valid;

    // We assume the mode list is the same when hashes are. So nothing more to compare.
    return simple_data_compare && global_data_compare;
}

int Output::id() const
{
    return d->id;
}

void Output::set_id(int id)
{
    d->id = id;
}

std::string Output::name() const
{
    return d->name;
}

void Output::set_name(std::string const& name)
{
    d->name = name;
}

std::string Output::description() const
{
    return d->description;
}

void Output::set_description(std::string const& description)
{
    d->description = description;
}

std::string Output::hash() const
{
    return d->hash;
}

void Output::set_hash(std::string const& input)
{
    auto const hash = QCryptographicHash::hash(input.c_str(), QCryptographicHash::Md5);
    d->hash = QString::fromLatin1(hash.toHex()).toStdString();
}

void Output::set_hash_raw(std::string const& hash)
{
    d->hash = hash;
}

Output::Type Output::type() const
{
    return d->type;
}

void Output::setType(Type type)
{
    d->type = type;
}

ModePtr Output::mode(std::string const& id) const
{
    if (d->modeList.find(id) == d->modeList.end()) {
        return ModePtr();
    }

    return d->modeList[id];
}

ModePtr Output::mode(QSize const& resolution, int refresh) const
{
    for (auto const& [key, mode] : d->modeList) {
        if (mode->size() == resolution && mode->refresh() == refresh) {
            return mode;
        }
    }

    return {};
}

ModeMap Output::modes() const
{
    return d->modeList;
}

void Output::set_modes(const ModeMap& modes)
{
    d->modeList = modes;
}

void Output::set_mode(ModePtr const& mode)
{
    set_resolution(mode->size());
    set_refresh_rate(mode->refresh());
}

void Output::set_to_preferred_mode()
{
    set_mode(preferred_mode());
}

ModePtr Output::commanded_mode() const
{
    for (auto const& [key, mode] : d->modeList) {
        if (mode->size() == d->resolution && mode->refresh() == d->refresh_rate) {
            return mode;
        }
    }
    return ModePtr();
}

bool Output::set_resolution(QSize const& size)
{
    d->resolution = size;
    return commanded_mode() != nullptr;
}

bool Output::set_refresh_rate(int rate)
{
    d->refresh_rate = rate;
    return commanded_mode() != nullptr;
}

QSize Output::best_resolution() const
{
    return d->best_resolution(modes());
}

int Output::best_refresh_rate(QSize const& resolution) const
{
    return d->best_refresh_rate(modes(), resolution);
}

ModePtr Output::best_mode() const
{
    return d->best_mode(modes());
}

ModePtr Output::auto_mode() const
{
    // Pick the preferred mode if the resolution and refresh rate is set to auto.
    // Picking the highest one causes issues for monitors that advertise preferred modes lower
    // than the highest available monitor (best example would be a 640x480 CRT that
    // exposes a 800x600 mode, but there's also a gaming monitor that gives a preferred
    // 144Hz mode, but exposes a 165Hz mode when "overclocking")
    if (auto_resolution() && auto_refresh_rate()) {
        return preferred_mode();
    }
    auto const resolution = auto_resolution() ? best_resolution() : d->resolution;
    auto const refresh_rate = auto_refresh_rate() ? best_refresh_rate(resolution) : d->refresh_rate;

    if (auto mode = d->mode(resolution, refresh_rate)) {
        return mode;
    }
    return preferred_mode();
}

void Output::set_preferred_modes(std::vector<std::string> const& modes)
{
    d->preferredMode = std::string();
    d->preferred_modes = modes;
}

std::vector<std::string> const& Output::preferred_modes() const
{
    return d->preferred_modes;
}

ModePtr Output::preferred_mode() const
{
    if (!d->preferredMode.empty()) {
        return d->modeList.at(d->preferredMode);
    }
    if (d->preferred_modes.empty()) {
        return d->best_mode(modes());
    }

    auto best = d->best_mode(d->preferred_modes);
    Q_ASSERT_X(best, "preferred_mode", "biggest mode must exist");

    d->preferredMode = best->id();
    return best;
}

void Output::set_position(const QPointF& position)
{
    d->position = position;
}

// TODO KF6: make the Rotation enum an enum class and align values with Wayland transformation
// property
Output::Rotation Output::rotation() const
{
    return d->rotation;
}

void Output::set_rotation(Output::Rotation rotation)
{
    d->rotation = rotation;
}

double Output::scale() const
{
    return d->scale;
}

void Output::set_scale(double scale)
{
    d->scale = scale;
}

QRectF Output::geometry() const
{
    auto geo = QRectF(d->position, QSizeF());

    if (d->enforced_geometry.isValid()) {
        return d->enforced_geometry;
    }

    auto const mode = auto_mode();
    if (!mode) {
        return geo;
    }

    auto size = mode->size();
    if (!size.isValid()) {
        return geo;
    }

    if (!horizontal()) {
        size = size.transposed();
    }

    geo.setSize(size / d->scale);
    return geo;
}

void Output::force_geometry(QRectF const& geo)
{
    d->enforced_geometry = geo;
}

QPointF Output::position() const
{
    return d->position;
}

bool Output::horizontal() const
{
    return ((rotation() == Output::None) || (rotation() == Output::Inverted));
}

bool Output::enabled() const
{
    return d->enabled;
}

void Output::set_enabled(bool enabled)
{
    d->enabled = enabled;
}

int Output::replication_source() const
{
    return d->replication_source;
}

void Output::set_replication_source(int source)
{
    d->replication_source = source;

    // Needs to be unset in case we run in-process. That value is not meant for consumption by
    // the frontend anyway.
    d->enforced_geometry = QRectF();
}

QSize Output::physical_size() const
{
    return d->physical_size;
}

void Output::set_physical_size(const QSize& size)
{
    d->physical_size = size;
}

bool Disman::Output::follow_preferred_mode() const
{
    return d->follow_preferred_mode;
}

void Disman::Output::set_follow_preferred_mode(bool follow)
{
    d->follow_preferred_mode = follow;
}

bool Output::auto_resolution() const
{
    return d->auto_resolution;
}

void Output::set_auto_resolution(bool auto_res)
{
    d->auto_resolution = auto_res;
}

bool Output::auto_refresh_rate() const
{
    return d->auto_refresh_rate;
}

void Output::set_auto_refresh_rate(bool auto_rate)
{
    d->auto_refresh_rate = auto_rate;
}

bool Output::auto_rotate() const
{
    return d->auto_rotate;
}

void Output::set_auto_rotate(bool auto_rot)
{
    d->auto_rotate = auto_rot;
}

bool Output::auto_rotate_only_in_tablet_mode() const
{
    return d->auto_rotate_only_in_tablet_mode;
}

void Output::set_auto_rotate_only_in_tablet_mode(bool only)
{
    d->auto_rotate_only_in_tablet_mode = only;
}

Output::Retention Output::retention() const
{
    return d->retention;
}

void Output::set_retention(Retention retention)
{
    d->retention = retention;
}

bool Output::positionable() const
{
    return enabled() && !replication_source();
}

void Output::apply(const OutputPtr& other)
{
    set_name(other->d->name);
    set_description(other->d->description);
    d->hash = other->d->hash;
    setType(other->d->type);
    set_position(other->geometry().topLeft());
    set_rotation(other->d->rotation);
    set_scale(other->d->scale);
    set_enabled(other->d->enabled);

    set_replication_source(other->d->replication_source);

    set_preferred_modes(other->d->preferred_modes);
    ModeMap modes;
    for (auto const& [key, mode] : other->modes()) {
        modes.insert({key, mode->clone()});
    }
    set_modes(modes);

    set_resolution(other->d->resolution);
    set_refresh_rate(other->d->refresh_rate);

    set_auto_resolution(other->d->auto_resolution);
    set_auto_refresh_rate(other->d->auto_refresh_rate);
    set_auto_rotate(other->d->auto_rotate);
    set_auto_rotate_only_in_tablet_mode(other->d->auto_rotate_only_in_tablet_mode);
    set_retention(other->d->retention);

    d->global = other->d->global;

    Q_EMIT updated();
}

Output::GlobalData Output::global_data() const
{
    return d->global;
}

void Output::set_global_data(GlobalData data)
{
    assert(data.resolution.isValid());
    assert(data.refresh > 0);
    assert(data.scale > 0);

    d->global = data;
    d->global.valid = data.resolution.isValid() && data.refresh > 0 && data.scale > 0;
}

std::string Output::log() const
{
    auto stream_size = [](QSizeF const& size) -> std::string {
        std::stringstream ss;
        ss << size.width() << "x" << size.height();
        return ss.str();
    };
    auto stream_mode = [&stream_size](Disman::ModePtr const& mode) {
        std::stringstream ss;
        if (mode) {
            ss << stream_size(mode->size()) << "@" << mode->refresh() << " (id " << mode->id()
               << ")";
        } else {
            ss << "null";
        }
        return ss.str();
    };
    auto stream_rect = [&stream_size](QRectF const& rect) {
        std::stringstream ss;
        ss << "x,y: " << rect.x() << "," << rect.y() << " w/h:" << stream_size(rect.size());
        return ss.str();
    };
    auto stream_geometry = [this, &stream_rect]() {
        std::stringstream ss;
        ss << stream_rect(geometry()) << " (scale: " << scale() << ")";
        return ss.str();
    };
    auto stream_retention = [](Retention retention) -> std::string {
        switch (retention) {
        case Retention::Global:
            return "global";
        case Retention::Individual:
            return "individual";
        case Retention::Undefined:
        default:
            return "undefined";
        }
    };
    auto stream_auto = [this]() -> std::string {
        std::string ret;
        auto comma{false};
        auto print = [&](auto check, std::string const& name) {
            if (!check) {
                return;
            }
            if (comma) {
                ret += ", ";
            }
            ret += name;
            comma = true;
        };
        print(auto_resolution(), "resolution");
        print(auto_refresh_rate(), "refresh");
        print(auto_rotate(), "rotate");
        if (auto_rotate() && auto_rotate_only_in_tablet_mode()) {
            ret += " (only in tablet mode)";
        }
        return ret;
    };
    auto stream_physical_size = [this, &stream_size]() -> std::string {
        std::stringstream ss;
        ss << "physical size[mm]: " << stream_size(physical_size());
        return ss.str();
    };
    auto stream_replication_source = [this]() -> std::string {
        auto source = replication_source();
        if (!source) {
            return "";
        }
        std::stringstream ss;
        ss << ", replicates: " << source;
        return ss.str();
    };

    auto const gap = std::string("  ");
    std::stringstream ss;

    ss << "Output " << id() << ", " << description() << " (" << name() << ")" << std::endl
       << gap << "mode: " << stream_mode(auto_mode()) << std::endl
       << gap << stream_geometry() << std::endl
       << gap << "auto: " << stream_auto() << std::endl
       << gap << stream_physical_size() << std::endl
       << gap << "enabled: " << (enabled() ? "yes" : "no")
       << ", retention: " << stream_retention(retention()) << stream_replication_source()
       << (follow_preferred_mode() ? ", hotplug-mode-update: QXL/SPICE" : "") << std::endl
       << gap << "hash: " << hash() << std::endl
       << "}";

    return ss.str();
}
}

QDebug operator<<(QDebug dbg, const Disman::OutputPtr& output)
{
    if (output) {
        dbg << output->log().c_str();
    } else {
        dbg << "Output {null}";
    }
    return dbg;
}
