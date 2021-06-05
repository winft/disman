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
#include "config.h"
#include "backend.h"
#include "backendmanager_p.h"
#include "disman_debug.h"
#include "output.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QRect>
#include <QStringList>

#include <sstream>

using namespace Disman;

class Q_DECL_HIDDEN Config::Private : public QObject
{
    Q_OBJECT
public:
    Private(Config* parent, Cause cause)
        : QObject(parent)
        , valid(true)
        , supported_features(Config::Feature::None)
        , tablet_mode_available(false)
        , tablet_mode_engaged(false)
        , cause{cause}
        , q(parent)
    {
    }

    auto remove_output(OutputMap::iterator iter)
    {
        if (iter == outputs.end()) {
            return iter;
        }

        OutputPtr output = iter->second;
        if (!output) {
            return outputs.erase(iter);
        }

        auto const outputId = iter->first;
        iter = outputs.erase(iter);

        if (primary_output == output) {
            q->set_primary_output(OutputPtr());
        }
        output->disconnect(q);

        Q_EMIT q->output_removed(outputId);

        return iter;
    }

    bool valid;
    ScreenPtr screen;
    OutputPtr primary_output;
    OutputMap outputs;
    Features supported_features;
    bool tablet_mode_available;
    bool tablet_mode_engaged;
    Cause cause;

private:
    Config* q;
};

bool Config::can_be_applied(const ConfigPtr& config)
{
    return can_be_applied(config, ValidityFlag::None);
}

bool Config::can_be_applied(const ConfigPtr& config, ValidityFlags flags)
{
    if (!config) {
        qCDebug(DISMAN) << "can_be_applied: Config not available, returning false";
        return false;
    }
    ConfigPtr currentConfig = BackendManager::instance()->config();
    if (!currentConfig) {
        qCDebug(DISMAN) << "can_be_applied: Current config not available, returning false";
        return false;
    }

    QRect rect;
    OutputPtr currentOutput;
    int enabledOutputsCount = 0;

    for (auto const& [key, output] : config->outputs()) {
        if (!output->enabled()) {
            continue;
        }

        ++enabledOutputsCount;

        currentOutput = currentConfig->output(output->id());
        // If there is no such output
        if (!currentOutput) {
            qCDebug(DISMAN) << "can_be_applied: The output:" << output->id() << "does not exists";
            return false;
        }

        // if there is no currentMode
        if (!output->auto_mode()) {
            qCDebug(DISMAN) << "can_be_applied: The output:" << output->id()
                            << "has no currentModeId";
            return false;
        }
        // If the mode is not found in the current output
        if (!currentOutput->mode(output->auto_mode()->id())) {
            qCDebug(DISMAN) << "can_be_applied: The output:" << output->id()
                            << "has no mode:" << output->auto_mode()->id().c_str();
            return false;
        }

        auto const outputSize = output->auto_mode()->size();

        if (output->position().x() < rect.x()) {
            rect.setX(output->position().x());
        }

        if (output->position().y() < rect.y()) {
            rect.setY(output->position().y());
        }

        QPoint bottomRight;
        if (output->horizontal()) {
            bottomRight = QPoint(output->position().x() + outputSize.width(),
                                 output->position().y() + outputSize.height());
        } else {
            bottomRight = QPoint(output->position().x() + outputSize.height(),
                                 output->position().y() + outputSize.width());
        }

        if (bottomRight.x() > rect.width()) {
            rect.setWidth(bottomRight.x());
        }

        if (bottomRight.y() > rect.height()) {
            rect.setHeight(bottomRight.y());
        }
    }

    if (flags & ValidityFlag::RequireAtLeastOneEnabledScreen && enabledOutputsCount == 0) {
        qCDebug(DISMAN) << "canBeAppled: There are no enabled screens, at least one required";
        return false;
    }

    const int maxEnabledOutputsCount = config->screen()->max_outputs_count();
    if (enabledOutputsCount > maxEnabledOutputsCount) {
        qCDebug(DISMAN) << "can_be_applied: Too many active screens. Requested: "
                        << enabledOutputsCount << ", Max: " << maxEnabledOutputsCount;
        return false;
    }

    // TODO: Why we do this again? Isn't the screen size determined by the outer rect of all
    //       outputs? At least it's that way in the Wayland case.
    if (rect.width() > config->screen()->max_size().width()) {
        qCDebug(DISMAN) << "can_be_applied: The configuration is too wide:" << rect.width();
        return false;
    }
    if (rect.height() > config->screen()->max_size().height()) {
        qCDebug(DISMAN) << "can_be_applied: The configuration is too high:" << rect.height();
        return false;
    }

    return true;
}

Config::Config()
    : Config(Cause::unknown)
{
}

Config::Config(Cause cause)
    : QObject(nullptr)
    , d(new Private(this, cause))
{
}

Config::~Config()
{
    delete d;
}

ConfigPtr Config::clone() const
{
    ConfigPtr newConfig(new Config(cause()));
    newConfig->d->screen = d->screen->clone();

    for (auto const& [key, ourOutput] : d->outputs) {
        auto cloned_output = ourOutput->clone();
        newConfig->add_output(cloned_output);

        if (d->primary_output == ourOutput) {
            newConfig->set_primary_output(cloned_output);
        }
    }

    newConfig->set_supported_features(supported_features());
    newConfig->set_tablet_mode_available(tablet_mode_available());
    newConfig->set_tablet_mode_engaged(tablet_mode_engaged());

    return newConfig;
}

bool Config::compare(ConfigPtr config) const
{
    if (!config) {
        return false;
    }

    auto const simple_data_compare = d->valid == config->d->valid && hash() == config->hash()
        && d->supported_features == config->d->supported_features
        && d->tablet_mode_available == config->d->tablet_mode_available
        && d->tablet_mode_engaged == config->d->tablet_mode_engaged && d->cause == config->d->cause;

    if (!simple_data_compare) {
        return false;
    }

    auto other_screen = config->screen();
    if (d->screen) {
        if (!other_screen || !d->screen->compare(other_screen)) {
            return false;
        }
    } else if (other_screen) {
        return false;
    }

    auto other_primary = config->primary_output();
    if (d->primary_output) {
        if (!other_primary || d->primary_output->id() != other_primary->id()) {
            return false;
        }
    } else if (other_primary) {
        return false;
    }

    // Check that we have all outputs of the other config.
    for (auto const& [key, output] : config->d->outputs) {
        if (!this->output(output->id())) {
            return false;
        }
    }
    // Check that the other config has also all our outputs and that these have the same data.
    for (auto const& [key, output] : d->outputs) {
        auto other_output = config->output(output->id());
        if (!other_output || !output->compare(other_output)) {
            return false;
        }
    }
    return true;
}

QString Config::hash() const
{
    QStringList hashedOutputs;
    for (auto const& [key, output] : d->outputs) {
        hashedOutputs << QString::fromStdString(output->hash());
    }

    std::sort(hashedOutputs.begin(), hashedOutputs.end());
    const auto hash = QCryptographicHash::hash(hashedOutputs.join(QString()).toLatin1(),
                                               QCryptographicHash::Md5);
    return QString::fromLatin1(hash.toHex());
}

Config::Cause Config::cause() const
{
    return d->cause;
}

void Config::set_cause(Cause cause)
{
    d->cause = cause;
}

ScreenPtr Config::screen() const
{
    return d->screen;
}

void Config::setScreen(const ScreenPtr& screen)
{
    d->screen = screen;
}

OutputPtr Config::output(int outputId) const
{
    if (auto out = d->outputs.find(outputId); out != d->outputs.end()) {
        return out->second;
    }
    return nullptr;
}

Config::Features Config::supported_features() const
{
    return d->supported_features;
}

void Config::set_supported_features(const Config::Features& features)
{
    d->supported_features = features;
}

bool Config::tablet_mode_available() const
{
    return d->tablet_mode_available;
}

void Config::set_tablet_mode_available(bool available)
{
    d->tablet_mode_available = available;
}

bool Config::tablet_mode_engaged() const
{
    return d->tablet_mode_engaged;
}

void Config::set_tablet_mode_engaged(bool engaged)
{
    d->tablet_mode_engaged = engaged;
}

OutputMap Config::outputs() const
{
    return d->outputs;
}

OutputPtr Config::primary_output() const
{
    return d->primary_output;
}

void Config::set_primary_output(const OutputPtr& newPrimary)
{
    if (d->primary_output == newPrimary) {
        return;
    }

    d->primary_output = newPrimary;
    Q_EMIT primary_output_changed(newPrimary);
}

OutputPtr Config::replication_source(OutputPtr const& output)
{
    if (auto source_id = output->replication_source()) {
        for (auto const& [key, output] : d->outputs) {
            if (output->id() == source_id) {
                return output;
            }
        }
    }
    return nullptr;
}

void Config::add_output(const OutputPtr& output)
{
    d->outputs.insert({output->id(), output});

    Q_EMIT output_added(output);
}

void Config::remove_output(int outputId)
{
    d->remove_output(d->outputs.find(outputId));
}

void Config::set_outputs(OutputMap const& outputs)
{
    auto primary = primary_output();
    for (auto iter = d->outputs.begin(), end = d->outputs.end(); iter != end;) {
        iter = d->remove_output(iter);
        end = d->outputs.end();
    }

    for (auto const& [key, output] : outputs) {
        add_output(output);
        if (primary && primary->id() == output->id()) {
            set_primary_output(output);
            primary = nullptr;
        }
    }
}

bool Config::valid() const
{
    return d->valid;
}

void Config::set_valid(bool valid)
{
    d->valid = valid;
}

void Config::apply(const ConfigPtr& other)
{
    d->screen->apply(other->screen());

    // Remove removed outputs
    for (auto iter = d->outputs.begin(), end = d->outputs.end(); iter != end;) {
        if (other->d->outputs.find(iter->second->id()) == other->d->outputs.end()) {
            iter = d->remove_output(iter);
            end = d->outputs.end();
        } else {
            iter++;
        }
    }

    for (auto const& [key, otherOutput] : other->d->outputs) {
        // Add new outputs
        OutputPtr output;
        auto primary
            = other->primary_output() && other->primary_output()->id() == otherOutput->id();

        if (d->outputs.find(otherOutput->id()) == d->outputs.end()) {
            output = otherOutput->clone();
            add_output(output);
        } else {
            // Update existing outputs
            output = d->outputs[otherOutput->id()];
            output->apply(otherOutput);
        }
        if (primary) {
            set_primary_output(output);
        }
    }

    // Update validity
    set_valid(other->valid());
    set_cause(other->cause());
}

std::string Config::log() const
{
    auto print_cause = [](Config::Cause cause) -> std::string {
        switch (cause) {
        case Config::Cause::unknown:
            return "unknown";
        case Config::Cause::file:
            return "file";
        case Config::Cause::generated:
            return "generated";
        case Config::Cause::interactive:
            return "interactive";
        default:
            return "ill-defined";
        }
    };

    std::stringstream ss;

    ss << "  Disman::Config {";
    ss << " cause: " << print_cause(cause());
    if (auto primary = primary_output()) {
        ss << ", primary: " << primary->id();
    }
    if (tablet_mode_available()) {
        ss << " tablet-mode: " << (tablet_mode_engaged() ? "engaged" : "disengaged");
    }
    for (auto const& [key, output] : outputs()) {
        auto log = std::istringstream(output->log());
        std::string line;
        while (std::getline(log, line)) {
            ss << std::endl << "    " << line;
        }
    }
    ss << std::endl << "  }";
    return ss.str();
}

QDebug operator<<(QDebug dbg, const Disman::ConfigPtr& config)
{
    if (config) {
        dbg << Qt::endl << config->log().c_str();
    } else {
        dbg << "Disman::Config {null}";
    }
    return dbg;
}

#include "config.moc"
