/*************************************************************************
Copyright © 2020   Roman Gilg <subdiff@gmail.com>

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
#include "generator.h"

#include <output.h>

#include "disman_debug.h"

#include <QRectF>

namespace Disman
{

Generator::Generator(ConfigPtr const& config)
    : m_config{config->clone()}
{
    prepare_config();
}

void Generator::prepare_config()
{
    auto outputs = m_config->outputs();

    for (auto& output : outputs) {
        // The scale will generally be independent no matter where the output is
        // scale will affect geometry, so do this first.
        if (m_config->supportedFeatures().testFlag(Disman::Config::Feature::PerOutputScaling)) {
            output->setScale(best_scale(output));
        }
        output->set_auto_resolution(true);
        output->set_auto_refresh_rate(true);
        output->setEnabled(true);
    }
}

void Generator::set_validities(Config::ValidityFlags validities)
{
    m_validities = validities;
}

ConfigPtr Generator::config() const
{
    return m_config;
}

bool Generator::optimize()
{
    assert(m_config);

    auto config = optimize_impl();

    if (!check_config(config)) {
        qCDebug(DISMAN) << "Config could not be optimized. Returning unaltered original config.";
        return false;
    }
    config->set_origin(Config::Origin::generated);

    qCDebug(DISMAN) << "Config optimized:" << config;
    m_config->apply(config);
    assert(check_config(m_config));
    return true;
}

bool Generator::extend(Extend_direction direction)
{
    return extend(nullptr, direction);
}

bool Generator::extend(OutputPtr const& first, Extend_direction direction)
{
    assert(m_config);

    auto config = m_config->clone();
    extend_impl(config, first, direction);

    if (!check_config(config)) {
        qCDebug(DISMAN) << "Could not extend. Config unchanged.";
        return false;
    }
    config->set_origin(Config::Origin::unknown);

    qCDebug(DISMAN) << "Generated extension configuration:" << config;
    m_config->apply(config);
    return true;
}

bool Generator::replicate()
{
    assert(m_config);

    auto config = m_config->clone();
    replicate_impl(config);

    if (!check_config(config)) {
        qCDebug(DISMAN) << "Could not replicate output. Config unchanged.";
        return false;
    }
    config->set_origin(Config::Origin::unknown);

    qCDebug(DISMAN) << "Generated replica configuration:" << config;
    m_config->apply(config);
    return true;
}

ConfigPtr Generator::optimize_impl()
{
    qCDebug(DISMAN) << "Generates ideal config for" << m_config->outputs().count() << "displays.";

    if (m_config->outputs().isEmpty()) {
        qCDebug(DISMAN) << "No displays connected. Nothing to generate.";
        return m_config;
    }

    auto config = m_config->clone();
    auto outputs = config->outputs();

    if (outputs.count() == 1) {
        single_output(config);
        return config;
    }

    extend_impl(config, nullptr, Extend_direction::right);

    return multi_output_fallback(config);
}

double Generator::best_scale(OutputPtr const& output)
{
    // If we have no physical size, we can't determine the DPI properly. Fallback to scale 1.
    if (output->sizeMm().height() <= 0) {
        return 1.0;
    }
    const auto mode = output->auto_mode();
    const qreal dpi = mode->size().height() / (output->sizeMm().height() / 25.4);

    // If reported DPI is closer to two times normal DPI, followed by a sanity check of having the
    // sort of vertical resolution you'd find in a high res screen.
    if (dpi > 96 * 1.5 && mode->size().height() >= 1440) {
        return 2.0;
    }
    return 1.0;
}

void Generator::single_output(ConfigPtr const& config)
{
    auto outputs = config->outputs();

    if (outputs.isEmpty()) {
        return;
    }

    auto output = outputs.value(outputs.keys().first());
    if (output->modes().isEmpty()) {
        return;
    }

    output->setPrimary(true);
    output->setPosition(QPointF(0, 0));
}

void Generator::extend_impl(ConfigPtr const& config,
                            OutputPtr const& first,
                            Extend_direction direction)
{
    assert(!first || first->isEnabled());

    auto outputs = config->outputs();

    qCDebug(DISMAN) << "Generate config by extending to the right.";

    if (outputs.isEmpty()) {
        qCDebug(DISMAN) << "No displays found. Nothing to generate.";
        return;
    }

    auto start_output = first ? first : primary_impl(outputs);
    assert(start_output);

    start_output->setEnabled(true);
    start_output->setPrimary(true);
    start_output->setPosition(QPointF(0, 0));

    double globalWidth = start_output->geometry().width();

    for (auto& output : outputs) {
        if (output == start_output) {
            continue;
        }
        output->setEnabled(true);
        output->setPrimary(false);
        output->setPosition(QPointF(globalWidth, 0));

        if (direction == Extend_direction::left) {
            globalWidth -= output->geometry().width();
        } else if (direction == Extend_direction::right) {
            globalWidth += output->geometry().width();
        } else {
            // We only have two directions at the moment.
            assert(false);
        }
    }
}

void Generator::replicate_impl(const ConfigPtr& config)
{
    auto outputs = config->outputs();

    auto source = primary_impl(outputs);
    source->setPrimary(true);

    qCDebug(DISMAN) << "Generate multi-output config by replicating" << source << "on"
                    << outputs.count() - 1 << "other outputs.";

    for (auto& output : outputs) {
        if (output == source) {
            continue;
        }
        output->setReplicationSource(source->id());
    }
}

ConfigPtr Generator::multi_output_fallback(ConfigPtr const& config)
{
    if (check_config(config)) {
        return config;
    }

    qCDebug(DISMAN) << "Ideal config can not be applied. Fallback to replicating outputs.";
    replicate_impl(config);

    return config;
}

bool Generator::check_config(ConfigPtr const& config)
{
    int enabled = 0;
    for (auto output : config->outputs()) {
        enabled += output->isEnabled();
    }
    if (m_validities & Config::ValidityFlag::RequireAtLeastOneEnabledScreen && enabled == 0) {
        qCDebug(DISMAN) << "Generator check failed: no enabled display, but required by flag.";
        return false;
    }
    return true;
}

OutputPtr Generator::primary() const
{
    return primary_impl(m_config->outputs());
}

OutputPtr Generator::embedded() const
{
    return embedded_impl(m_config->outputs());
}

OutputPtr Generator::biggest() const
{
    return biggest_impl(m_config->outputs(), false);
}

OutputPtr Generator::primary_impl(OutputList const& outputs) const
{
    // If one of the outputs is a embedded (panel) display, then we take this one as primary.
    if (auto output = embedded_impl(outputs)) {
        if (output->isEnabled()) {
            return output;
        }
    }
    return biggest_impl(outputs, true);
}

OutputPtr Generator::embedded_impl(OutputList const& outputs) const
{
    auto it = std::find_if(outputs.constBegin(), outputs.constEnd(), [](OutputPtr const& output) {
        return output->type() == Output::Panel;
    });
    return it != outputs.constEnd() ? *it : OutputPtr();
}

OutputPtr Generator::biggest_impl(OutputList const& outputs, bool only_enabled) const
{
    auto max_area = 0;
    OutputPtr biggest;

    for (auto const& output : outputs) {
        auto const mode = output->best_mode();
        if (!mode) {
            continue;
        }
        if (only_enabled && !output->isEnabled()) {
            continue;
        }
        auto const area = mode->size().width() * mode->size().height();
        if (area > max_area) {
            max_area = area;
            biggest = output;
        }
    }

    return biggest;
}

}
