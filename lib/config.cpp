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
#include "abstractbackend.h"
#include "backendmanager_p.h"
#include "disman_debug.h"
#include "output.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QRect>
#include <QStringList>

using namespace Disman;

class Q_DECL_HIDDEN Config::Private : public QObject
{
    Q_OBJECT
public:
    Private(Config* parent, Origin origin)
        : QObject(parent)
        , valid(true)
        , supportedFeatures(Config::Feature::None)
        , tabletModeAvailable(false)
        , tabletModeEngaged(false)
        , origin{origin}
        , q(parent)
    {
    }

    OutputList::Iterator removeOutput(OutputList::Iterator iter)
    {
        if (iter == outputs.end()) {
            return iter;
        }

        OutputPtr output = iter.value();
        if (!output) {
            return outputs.erase(iter);
        }

        const int outputId = iter.key();
        iter = outputs.erase(iter);

        if (primaryOutput == output) {
            q->setPrimaryOutput(OutputPtr());
        }
        output->disconnect(q);

        Q_EMIT q->outputRemoved(outputId);

        return iter;
    }

    bool valid;
    ScreenPtr screen;
    OutputPtr primaryOutput;
    OutputList outputs;
    Features supportedFeatures;
    bool tabletModeAvailable;
    bool tabletModeEngaged;
    Origin origin;

private:
    Config* q;
};

bool Config::canBeApplied(const ConfigPtr& config)
{
    return canBeApplied(config, ValidityFlag::None);
}

bool Config::canBeApplied(const ConfigPtr& config, ValidityFlags flags)
{
    if (!config) {
        qCDebug(DISMAN) << "canBeApplied: Config not available, returning false";
        return false;
    }
    ConfigPtr currentConfig = BackendManager::instance()->config();
    if (!currentConfig) {
        qCDebug(DISMAN) << "canBeApplied: Current config not available, returning false";
        return false;
    }

    QRect rect;
    OutputPtr currentOutput;
    const OutputList outputs = config->outputs();
    int enabledOutputsCount = 0;
    Q_FOREACH (const OutputPtr& output, outputs) {
        if (!output->isEnabled()) {
            continue;
        }

        ++enabledOutputsCount;

        currentOutput = currentConfig->output(output->id());
        // If there is no such output
        if (!currentOutput) {
            qCDebug(DISMAN) << "canBeApplied: The output:" << output->id() << "does not exists";
            return false;
        }
        // if there is no currentMode
        if (output->auto_mode().isNull()) {
            qCDebug(DISMAN) << "canBeApplied: The output:" << output->id()
                            << "has no currentModeId";
            return false;
        }
        // If the mode is not found in the current output
        if (!currentOutput->mode(output->auto_mode()->id())) {
            qCDebug(DISMAN) << "canBeApplied: The output:" << output->id()
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
        if (output->isHorizontal()) {
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

    const int maxEnabledOutputsCount = config->screen()->maxActiveOutputsCount();
    if (enabledOutputsCount > maxEnabledOutputsCount) {
        qCDebug(DISMAN) << "canBeApplied: Too many active screens. Requested: "
                        << enabledOutputsCount << ", Max: " << maxEnabledOutputsCount;
        return false;
    }

    // TODO: Why we do this again? Isn't the screen size determined by the outer rect of all
    //       outputs? At least it's that way in the Wayland case.
    if (rect.width() > config->screen()->maxSize().width()) {
        qCDebug(DISMAN) << "canBeApplied: The configuration is too wide:" << rect.width();
        return false;
    }
    if (rect.height() > config->screen()->maxSize().height()) {
        qCDebug(DISMAN) << "canBeApplied: The configuration is too high:" << rect.height();
        return false;
    }

    return true;
}

Config::Config()
    : Config(Origin::unknown)
{
}

Config::Config(Origin origin)
    : QObject(nullptr)
    , d(new Private(this, origin))
{
}

Config::~Config()
{
    delete d;
}

ConfigPtr Config::clone() const
{
    ConfigPtr newConfig(new Config(origin()));
    newConfig->d->screen = d->screen->clone();

    for (const OutputPtr& ourOutput : d->outputs) {
        auto cloned_output = ourOutput->clone();
        newConfig->addOutput(cloned_output);

        if (d->primaryOutput == ourOutput) {
            newConfig->setPrimaryOutput(cloned_output);
        }
    }

    newConfig->setSupportedFeatures(supportedFeatures());
    newConfig->setTabletModeAvailable(tabletModeAvailable());
    newConfig->setTabletModeEngaged(tabletModeEngaged());

    return newConfig;
}

QString Config::connectedOutputsHash() const
{
    QStringList hashedOutputs;
    for (OutputPtr const& output : d->outputs) {
        hashedOutputs << QString::fromStdString(output->hash());
    }

    std::sort(hashedOutputs.begin(), hashedOutputs.end());
    const auto hash = QCryptographicHash::hash(hashedOutputs.join(QString()).toLatin1(),
                                               QCryptographicHash::Md5);
    return QString::fromLatin1(hash.toHex());
}

Config::Origin Config::origin() const
{
    return d->origin;
}

void Config::set_origin(Origin origin)
{
    d->origin = origin;
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
    return d->outputs.value(outputId);
}

Config::Features Config::supportedFeatures() const
{
    return d->supportedFeatures;
}

void Config::setSupportedFeatures(const Config::Features& features)
{
    d->supportedFeatures = features;
}

bool Config::tabletModeAvailable() const
{
    return d->tabletModeAvailable;
}

void Config::setTabletModeAvailable(bool available)
{
    d->tabletModeAvailable = available;
}

bool Config::tabletModeEngaged() const
{
    return d->tabletModeEngaged;
}

void Config::setTabletModeEngaged(bool engaged)
{
    d->tabletModeEngaged = engaged;
}

OutputList Config::outputs() const
{
    return d->outputs;
}

OutputPtr Config::primaryOutput() const
{
    return d->primaryOutput;
}

void Config::setPrimaryOutput(const OutputPtr& newPrimary)
{
    if (d->primaryOutput == newPrimary) {
        return;
    }

    d->primaryOutput = newPrimary;
    Q_EMIT primaryOutputChanged(newPrimary);
}

OutputPtr Config::replication_source(OutputPtr const& output)
{
    if (auto source_id = output->replicationSource()) {
        for (auto const& output : d->outputs) {
            if (output->id() == source_id) {
                return output;
            }
        }
    }
    return nullptr;
}

void Config::addOutput(const OutputPtr& output)
{
    d->outputs.insert(output->id(), output);

    Q_EMIT outputAdded(output);
}

void Config::removeOutput(int outputId)
{
    d->removeOutput(d->outputs.find(outputId));
}

void Config::setOutputs(const OutputList& outputs)
{
    auto primary = primaryOutput();
    for (auto iter = d->outputs.begin(), end = d->outputs.end(); iter != end;) {
        iter = d->removeOutput(iter);
        end = d->outputs.end();
    }

    for (const OutputPtr& output : outputs) {
        addOutput(output);
        if (primary && primary->id() == output->id()) {
            setPrimaryOutput(output);
            primary = nullptr;
        }
    }
}

bool Config::isValid() const
{
    return d->valid;
}

void Config::setValid(bool valid)
{
    d->valid = valid;
}

void Config::apply(const ConfigPtr& other)
{
    d->screen->apply(other->screen());

    // Remove removed outputs
    Q_FOREACH (const OutputPtr& output, d->outputs) {
        if (!other->d->outputs.contains(output->id())) {
            removeOutput(output->id());
        }
    }

    Q_FOREACH (const OutputPtr& otherOutput, other->d->outputs) {
        // Add new outputs
        OutputPtr output;
        auto primary = other->primaryOutput() && other->primaryOutput()->id() == otherOutput->id();

        if (!d->outputs.contains(otherOutput->id())) {
            output = otherOutput->clone();
            addOutput(output);
        } else {
            // Update existing outputs
            output = d->outputs[otherOutput->id()];
            output->apply(otherOutput);
        }
        if (primary) {
            setPrimaryOutput(output);
        }
    }

    // Update validity
    setValid(other->isValid());
}

QDebug operator<<(QDebug dbg, const Disman::ConfigPtr& config)
{
    if (config) {
        dbg << "Disman::Config(";
        if (auto primary = config->primaryOutput()) {
            dbg << "Primary:" << primary->id();
        }
        const auto outputs = config->outputs();
        for (const auto& output : outputs) {
            dbg << Qt::endl << output;
        }
        dbg << ")";
    } else {
        dbg << "Disman::Config(NULL)";
    }
    return dbg;
}

#include "config.moc"
