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
    Private(Config* parent)
        : QObject(parent)
        , valid(true)
        , supportedFeatures(Config::Feature::None)
        , tabletModeAvailable(false)
        , tabletModeEngaged(false)
        , q(parent)
    {
    }

    Disman::OutputPtr findPrimaryOutput() const
    {
        auto iter = std::find_if(
            outputs.constBegin(), outputs.constEnd(), [](const Disman::OutputPtr& output) -> bool {
                return output->isPrimary();
            });
        return iter == outputs.constEnd() ? Disman::OutputPtr() : iter.value();
    }

    void onPrimaryOutputChanged()
    {
        const Disman::OutputPtr output(qobject_cast<Disman::Output*>(sender()), [](void*) {});
        Q_ASSERT(output);
        if (output->isPrimary()) {
            q->setPrimaryOutput(output);
        } else {
            q->setPrimaryOutput(findPrimaryOutput());
        }
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
        // If the output is not connected
        if (!currentOutput->isConnected()) {
            qCDebug(DISMAN) << "canBeApplied: The output:" << output->id() << "is not connected";
            return false;
        }
        // if there is no currentMode
        if (output->currentModeId().isEmpty()) {
            qCDebug(DISMAN) << "canBeApplied: The output:" << output->id()
                            << "has no currentModeId";
            return false;
        }
        // If the mode is not found in the current output
        if (!currentOutput->mode(output->currentModeId())) {
            qCDebug(DISMAN) << "canBeApplied: The output:" << output->id()
                            << "has no mode:" << output->currentModeId();
            return false;
        }

        auto const outputSize = output->currentMode()->size();

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
    : QObject(nullptr)
    , d(new Private(this))
{
}

Config::~Config()
{
    delete d;
}

ConfigPtr Config::clone() const
{
    ConfigPtr newConfig(new Config());
    newConfig->d->screen = d->screen->clone();
    for (const OutputPtr& ourOutput : d->outputs) {
        newConfig->addOutput(ourOutput->clone());
    }
    newConfig->d->primaryOutput = newConfig->d->findPrimaryOutput();
    newConfig->setSupportedFeatures(supportedFeatures());
    newConfig->setTabletModeAvailable(tabletModeAvailable());
    newConfig->setTabletModeEngaged(tabletModeEngaged());
    return newConfig;
}

// TODO: take Output::hashMd5() values
QString Config::connectedOutputsHash() const
{
    QStringList hashedOutputs;

    const auto outputs = connectedOutputs();
    for (const OutputPtr& output : outputs) {
        hashedOutputs << output->hash();
    }
    std::sort(hashedOutputs.begin(), hashedOutputs.end());
    const auto hash = QCryptographicHash::hash(hashedOutputs.join(QString()).toLatin1(),
                                               QCryptographicHash::Md5);
    return QString::fromLatin1(hash.toHex());
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

OutputList Config::connectedOutputs() const
{
    OutputList outputs;
    Q_FOREACH (const OutputPtr& output, d->outputs) {
        if (!output->isConnected()) {
            continue;
        }
        outputs.insert(output->id(), output);
    }

    return outputs;
}

OutputPtr Config::primaryOutput() const
{
    if (d->primaryOutput) {
        return d->primaryOutput;
    }

    d->primaryOutput = d->findPrimaryOutput();
    return d->primaryOutput;
}

void Config::setPrimaryOutput(const OutputPtr& newPrimary)
{
    // Don't call primaryOutput(): at this point d->primaryOutput is either
    // initialized, or we need to look for the primary anyway
    if (d->primaryOutput == newPrimary) {
        return;
    }

    //     qCDebug(DISMAN) << "Primary output changed from" << primaryOutput()
    //                      << "(" << (primaryOutput().isNull() ? "none" : primaryOutput()->name())
    //                      << ") to"
    //                      << newPrimary << "(" << (newPrimary.isNull() ? "none" :
    //                      newPrimary->name()) << ")";

    for (OutputPtr& output : d->outputs) {
        disconnect(output.data(),
                   &Disman::Output::isPrimaryChanged,
                   d,
                   &Disman::Config::Private::onPrimaryOutputChanged);
        output->setPrimary(output == newPrimary);
        connect(output.data(),
                &Disman::Output::isPrimaryChanged,
                d,
                &Disman::Config::Private::onPrimaryOutputChanged);
    }

    d->primaryOutput = newPrimary;
    Q_EMIT primaryOutputChanged(newPrimary);
}

void Config::addOutput(const OutputPtr& output)
{
    d->outputs.insert(output->id(), output);
    connect(output.data(),
            &Disman::Output::isPrimaryChanged,
            d,
            &Disman::Config::Private::onPrimaryOutputChanged);

    Q_EMIT outputAdded(output);

    if (output->isPrimary()) {
        setPrimaryOutput(output);
    }
}

void Config::removeOutput(int outputId)
{
    d->removeOutput(d->outputs.find(outputId));
}

void Config::setOutputs(const OutputList& outputs)
{
    for (auto iter = d->outputs.begin(), end = d->outputs.end(); iter != end;) {
        iter = d->removeOutput(iter);
        end = d->outputs.end();
    }

    for (const OutputPtr& output : outputs) {
        addOutput(output);
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
        if (!d->outputs.contains(otherOutput->id())) {
            addOutput(otherOutput->clone());
        } else {
            // Update existing outputs
            d->outputs[otherOutput->id()]->apply(otherOutput);
        }
    }

    // Update validity
    setValid(other->isValid());
}

QDebug operator<<(QDebug dbg, const Disman::ConfigPtr& config)
{
    if (config) {
        dbg << "Disman::Config(";
        const auto outputs = config->outputs();
        for (const auto& output : outputs) {
            if (output->isConnected()) {
                dbg << endl << output;
            }
        }
        dbg << ")";
    } else {
        dbg << "Disman::Config(NULL)";
    }
    return dbg;
}

#include "config.moc"
