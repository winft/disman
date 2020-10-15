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

#include "output_p.h"

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

void Generator::prepare_config()
{
    for (auto const& [key, output] : m_config->outputs()) {
        if (output->d->global.valid) {
            // We have global data for the output. We fall back to these values if necessary.
            continue;
        }

        // The scale will generally be independent no matter where the output is
        // scale will affect geometry, so do this first.
        if (m_config->supported_features().testFlag(Disman::Config::Feature::PerOutputScaling)) {
            output->set_scale(best_scale(output));
        }
        output->set_auto_resolution(true);
        output->set_auto_refresh_rate(true);
        output->set_enabled(true);
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
    config->set_cause(Config::Cause::generated);

    qCDebug(DISMAN) << "Config optimized:" << config;
    m_config->apply(config);
    assert(check_config(m_config));
    return true;
}

void normalize_positions(ConfigPtr& config)
{
    double min_x = 0;
    double min_y = 0;
    bool is_set = false;

    for (auto const& [key, output] : config->outputs()) {
        if (!output->positionable()) {
            continue;
        }

        auto const x = output->position().x();
        auto const y = output->position().y();
        if (!is_set) {
            min_x = x;
            min_y = y;
            is_set = true;
        }

        if (x < min_x) {
            min_x = x;
        }
        if (y < min_y) {
            min_y = y;
        }
    }

    for (auto& [key, output] : config->outputs()) {
        auto const pos = output->position();
        output->set_position(QPointF(pos.x() - min_x, pos.y() - min_y));
    }
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
    config->set_cause(Config::Cause::unknown);

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
    config->set_cause(Config::Cause::unknown);

    qCDebug(DISMAN) << "Generated replica configuration:" << config;
    m_config->apply(config);
    return true;
}

bool Generator::disable_embedded()
{
    assert(m_config);
    auto config = m_config->clone();

    auto embedded = embedded_impl(config->outputs(), OutputMap());
    if (!embedded) {
        qCWarning(DISMAN) << "No embedded output found to disable. Config unchanged.";
        return false;
    }

    auto biggest_external = biggest_impl(config->outputs(), false, {{embedded->id(), embedded}});
    if (!biggest_external) {
        qCWarning(DISMAN) << "No external output found when disabling embedded. Config unchanged.";
        return false;
    }

    qCDebug(DISMAN) << "Disable embedded:" << embedded;
    qCDebug(DISMAN) << "Enable external:" << biggest_external;

    embedded->set_enabled(false);
    biggest_external->set_enabled(true);

    // TODO: reorder positions.
    normalize_positions(config);

    if (!check_config(config)) {
        qCWarning(DISMAN) << "Could not disable embedded output. Config unchanged.";
        return false;
    }

    m_config->apply(config);
    return true;
}

ConfigPtr Generator::optimize_impl()
{
    qCDebug(DISMAN) << "Generates ideal config for" << m_config->outputs().size() << "displays.";

    if (m_config->outputs().empty()) {
        qCDebug(DISMAN) << "No displays connected. Nothing to generate.";
        return m_config;
    }

    auto config = m_config->clone();
    auto outputs = config->outputs();

    if (outputs.size() == 1) {
        single_output(config);
        return config;
    }

    extend_impl(config, nullptr, Extend_direction::right);

    return multi_output_fallback(config);
}

double Generator::best_scale(OutputPtr const& output)
{
    // If we have no physical size, we can't determine the DPI properly. Fallback to scale 1.
    if (output->physical_size().height() <= 0) {
        return 1.0;
    }

    const auto mode = output->auto_mode();
    const qreal dpi = mode->size().height() / (output->physical_size().height() / 25.4);

    // We see 110 DPI as a good standard. That corresponds to 1440p at 23" and 2160p/UHD at 34".
    // This is smaller than usual but with high DPI screens this is often easily possible and
    // otherwise we just don't scale at the moment.
    auto scale_factor = dpi / 130;

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

    if (outputs.empty()) {
        return;
    }

    auto output = outputs.begin()->second;
    if (output->modes().empty()) {
        return;
    }

    if (config->supported_features().testFlag(Config::Feature::PrimaryDisplay)) {
        config->set_primary_output(output);
    }

    output->set_position(QPointF(0, 0));

    output->d->apply_global();
}

void Generator::extend_impl(ConfigPtr const& config,
                            OutputPtr const& first,
                            Extend_direction direction)
{
    assert(!first || first->enabled());

    auto outputs = config->outputs();

    qCDebug(DISMAN) << "Generate config by extending to the right.";

    if (outputs.empty()) {
        qCDebug(DISMAN) << "No displays found. Nothing to generate.";
        return;
    }

    auto start_output = first ? first : primary_impl(outputs, OutputMap());
    if (!start_output) {
        qCDebug(DISMAN) << "No displays enabled. Nothing to generate.";
        return;
    }

    if (config->supported_features().testFlag(Config::Feature::PrimaryDisplay)
        && config->primary_output() == nullptr) {
        config->set_primary_output(start_output);
    }

    line_up(start_output, OutputMap(), outputs, direction);
}

void Generator::get_outputs_division(OutputPtr const& first,
                                     ConfigPtr const& config,
                                     OutputMap& old_outputs,
                                     OutputMap& new_outputs)
{
    OutputPtr recent_output;

    for (auto const& [key, output] : config->outputs()) {
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
        old_outputs.erase(recent_output->id());
    }
}

void Generator::line_up(OutputPtr const& first,
                        OutputMap const& old_outputs,
                        OutputMap const& new_outputs,
                        Extend_direction direction)
{
    first->set_position(QPointF(0, 0));
    first->d->apply_global();

    double globalWidth
        = direction == Extend_direction::right ? first->geometry().width() : first->position().x();

    for (auto const& [key, output] : old_outputs) {
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

    for (auto& [key, output] : new_outputs) {
        if (output->id() == first->id()) {
            continue;
        }

        output->d->apply_global();

        if (direction == Extend_direction::left) {
            globalWidth -= output->geometry().width();
            output->set_position(QPointF(globalWidth, 0));
        } else if (direction == Extend_direction::right) {
            output->set_position(QPointF(globalWidth, 0));
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

    auto source = primary_impl(outputs, OutputMap());
    source->d->apply_global();

    // TODO: Do this only if config supports primary output and there is not a primary output that
    //       independent of this replication.
    config->set_primary_output(source);

    qCDebug(DISMAN) << "Generate multi-output config by replicating" << source << "on"
                    << outputs.size() - 1 << "other outputs.";

    for (auto& [key, output] : outputs) {
        if (output == source) {
            continue;
        }

        output->d->apply_global();
        output->set_replication_source(source->id());
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
    for (auto const& [key, output] : config->outputs()) {
        enabled += output->enabled();
    }
    if (m_validities & Config::ValidityFlag::RequireAtLeastOneEnabledScreen && enabled == 0) {
        qCDebug(DISMAN) << "Generator check failed: no enabled display, but required by flag.";
        return false;
    }

    // TODO: check for primary output being set when primary output supported.
    return true;
}

OutputPtr Generator::primary(OutputMap const& exclusions) const
{
    return primary_impl(m_config->outputs(), exclusions);
}

OutputPtr Generator::embedded() const
{
    return embedded_impl(m_config->outputs(), OutputMap());
}

OutputPtr Generator::biggest(OutputMap const& exclusions) const
{
    return biggest_impl(m_config->outputs(), false, exclusions);
}

OutputPtr Generator::primary_impl(OutputMap const& outputs, OutputMap const& exclusions) const
{
    // If one of the outputs is a embedded (panel) display, then we take this one as primary.
    if (auto output = embedded_impl(outputs, exclusions)) {
        if (output->enabled()) {
            return output;
        }
    }
    return biggest_impl(outputs, true, exclusions);
}

OutputPtr Generator::embedded_impl(OutputMap const& outputs, OutputMap const& exclusions) const
{
    auto it = std::find_if(outputs.cbegin(), outputs.cend(), [&exclusions](auto const& output) {
        return output.second->type() == Output::Panel
            && exclusions.find(output.second->id()) == exclusions.end();
    });
    return it != outputs.cend() ? it->second : OutputPtr();
}

OutputPtr Generator::biggest_impl(OutputMap const& outputs,
                                  bool only_enabled,
                                  const OutputMap& exclusions) const
{
    auto max_area = 0;
    OutputPtr biggest;

    for (auto const& [key, output] : outputs) {
        if (exclusions.find(output->id()) != exclusions.end()) {
            continue;
        }
        auto const mode = output->best_mode();
        if (!mode) {
            continue;
        }
        if (only_enabled && !output->enabled()) {
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
