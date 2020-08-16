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
    , m_predecessor_config{config}
{
    prepare_config();
}

Generator::Generator(ConfigPtr const& config, ConfigPtr const& predecessor)
    : m_config{config->clone()}
    , m_predecessor_config{predecessor}
{
    prepare_config();
    set_derived();
}

void Generator::set_derived()
{
    if (!m_predecessor_config) {
        return;
    }

    m_derived = true;

    for (auto output : m_config->outputs()) {
        if (auto predecessor_output = m_predecessor_config->output(output->id())) {
            output->apply(predecessor_output);
        }
    }
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

    // We see 150 DPI as a good standard. That corresponds to 1440p at 20" and 2160p/UHD at 30".
    // This is smaller than usual but with high dpi screens this is often easily possible and
    // otherwise we just don't scale at the moment.
    auto scale_factor = dpi / 150;

    // We only auto-scale displays up.
    if (scale_factor < 1) {
        return 1.;
    }

    // We only auto-scale with one digit.
    scale_factor = static_cast<int>(scale_factor * 10 + 0.5) / 10.;

    // And only up to maximal 3 times.
    if (scale_factor > 3) {
        return 3.;
    }

    return scale_factor;
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

    auto start_output = first ? first : primary_impl(outputs, OutputList());
    if (!start_output) {
        qCDebug(DISMAN) << "No displays enabled. Nothing to generate.";
        return;
    }

    if (m_derived) {
        extend_derived(config, start_output, direction);
        return;
    }

    line_up(start_output, OutputList(), outputs, direction);
}

void Generator::extend_derived(ConfigPtr const& config,
                               OutputPtr const& first,
                               Extend_direction direction)
{
    OutputList old_outputs;
    OutputList new_outputs;

    get_outputs_division(first, config, old_outputs, new_outputs);
    line_up(first, old_outputs, new_outputs, direction);
}

void Generator::get_outputs_division(OutputPtr const& first,
                                     ConfigPtr const& config,
                                     OutputList& old_outputs,
                                     OutputList& new_outputs)
{
    OutputPtr recent_output;

    for (auto output : config->outputs()) {
        if (output == first) {
            continue;
        }
        if (m_predecessor_config->output(output->id())) {
            old_outputs[output->id()] = output;
        } else {
            new_outputs[output->id()] = output;
        }
        if (!recent_output || recent_output->id() < output->id()) {
            recent_output = output;
        }
    }

    if (!new_outputs.size()) {
        // If we have no new outputs we assume the last one added (not the one designated as being
        // first) should be extended in the given direction.
        new_outputs[recent_output->id()] = recent_output;
        old_outputs.remove(recent_output->id());
    }
}

void Generator::line_up(OutputPtr const& first,
                        OutputList const& old_outputs,
                        OutputList const& new_outputs,
                        Extend_direction direction)
{
    first->setPrimary(true);
    first->setPosition(QPointF(0, 0));

    double globalWidth
        = direction == Extend_direction::right ? first->geometry().width() : first->position().x();

    for (auto& output : old_outputs) {
        if (direction == Extend_direction::left) {
            auto const left = output->position().x();
            if (left < globalWidth) {
                globalWidth = left;
            }
        } else if (direction == Extend_direction::right) {
            auto const right = output->position().x() + output->geometry().width();
            if (right > globalWidth) {
                globalWidth = right;
            }
        } else {
            // We only have two directions at the moment.
            assert(false);
        }
    }

    for (auto& output : new_outputs) {
        if (output->id() == first->id()) {
            continue;
        }

        if (direction == Extend_direction::left) {
            globalWidth -= output->geometry().width();
            output->setPosition(QPointF(globalWidth, 0));
        } else if (direction == Extend_direction::right) {
            output->setPosition(QPointF(globalWidth, 0));
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

    auto source = primary_impl(outputs, OutputList());
    source->setPrimary(true);

    if (m_derived) {
        replicate_derived(config, source);
        return;
    }

    qCDebug(DISMAN) << "Generate multi-output config by replicating" << source << "on"
                    << outputs.count() - 1 << "other outputs.";

    for (auto& output : outputs) {
        if (output == source) {
            continue;
        }
        output->setReplicationSource(source->id());
    }
}

void Generator::replicate_derived(ConfigPtr const& config, OutputPtr const& source)
{
    OutputList old_outputs;
    OutputList new_outputs;

    get_outputs_division(source, config, old_outputs, new_outputs);

    for (auto output : new_outputs) {
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

OutputPtr Generator::primary(OutputList const& exclusions) const
{
    return primary_impl(m_config->outputs(), exclusions);
}

OutputPtr Generator::embedded() const
{
    return embedded_impl(m_config->outputs(), OutputList());
}

OutputPtr Generator::biggest(OutputList const& exclusions) const
{
    return biggest_impl(m_config->outputs(), false, exclusions);
}

OutputPtr Generator::primary_impl(OutputList const& outputs, OutputList const& exclusions) const
{
    if (auto output = m_config->primaryOutput()) {
        if (m_derived && !exclusions.contains(output->id())) {
            return output;
        }
    }
    // If one of the outputs is a embedded (panel) display, then we take this one as primary.
    if (auto output = embedded_impl(outputs, exclusions)) {
        if (output->isEnabled()) {
            return output;
        }
    }
    return biggest_impl(outputs, true, exclusions);
}

OutputPtr Generator::embedded_impl(OutputList const& outputs, OutputList const& exclusions) const
{
    auto it = std::find_if(
        outputs.constBegin(), outputs.constEnd(), [&exclusions](OutputPtr const& output) {
            return output->type() == Output::Panel && !exclusions.contains(output->id());
        });
    return it != outputs.constEnd() ? *it : OutputPtr();
}

OutputPtr Generator::biggest_impl(OutputList const& outputs,
                                  bool only_enabled,
                                  const OutputList& exclusions) const
{
    auto max_area = 0;
    OutputPtr biggest;

    for (auto const& output : outputs) {
        if (exclusions.contains(output->id())) {
            continue;
        }
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
