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
#pragma once

#include "config.h"
#include "types.h"

#include "disman_export.h"

namespace Disman
{

/**
 * Generic Config generator that also provides
 * TODO
 */
class DISMAN_EXPORT Generator
{
public:
    enum class Extend_direction {
        left,
        right,
    };

    explicit Generator(ConfigPtr const& config);

    void set_validities(Config::ValidityFlags validities);

    ConfigPtr config() const;

    bool optimize();
    bool extend(Extend_direction direction);
    bool extend(OutputPtr const& first, Extend_direction direction);
    bool replicate();
    bool disable_embedded();

    OutputPtr primary(OutputMap const& exclusions = OutputMap()) const;
    OutputPtr embedded() const;
    OutputPtr biggest(OutputMap const& exclusions = OutputMap()) const;

    double best_scale(OutputPtr const& output);

private:
    void prepare_config();
    bool check_config(ConfigPtr const& config);

    ConfigPtr optimize_impl();

    void single_output(ConfigPtr const& config);

    void extend_impl(ConfigPtr const& config, OutputPtr const& first, Extend_direction direction);
    void line_up(OutputPtr const& first,
                 OutputMap const& old_outputs,
                 OutputMap const& new_outputs,
                 Extend_direction direction);

    void replicate_impl(ConfigPtr const& config);

    ConfigPtr multi_output_fallback(ConfigPtr const& config);

    OutputPtr primary_impl(OutputMap const& outputs, OutputMap const& exclusions) const;
    OutputPtr embedded_impl(OutputMap const& outputs, OutputMap const& exclusions) const;
    OutputPtr
    biggest_impl(OutputMap const& outputs, bool only_enabled, OutputMap const& exclusions) const;

    void get_outputs_division(OutputPtr const& first,
                              const ConfigPtr& config,
                              OutputMap& old_outputs,
                              OutputMap& new_outputs);

    ConfigPtr m_config;
    ConfigPtr m_predecessor_config;
    Config::ValidityFlags m_validities;
};

}
