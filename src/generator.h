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

    OutputPtr primary() const;
    OutputPtr embedded() const;
    OutputPtr biggest() const;

    double best_scale(OutputPtr const& output);

private:
    void prepare_config();
    bool check_config(ConfigPtr const& config);

    ConfigPtr optimize_impl();

    OutputPtr primary_impl(OutputList const& outputs) const;
    OutputPtr embedded_impl(OutputList const& outputs) const;
    OutputPtr biggest_impl(OutputList const& outputs, bool only_enabled) const;

    void single_output(ConfigPtr const& config);
    void extend_impl(ConfigPtr const& config, OutputPtr const& first, Extend_direction direction);
    void replicate_impl(ConfigPtr const& config);

    ConfigPtr multi_output_fallback(ConfigPtr const& config);

    ConfigPtr m_config;
    Config::ValidityFlags m_validities;
};

}
