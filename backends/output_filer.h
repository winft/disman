/*************************************************************************
Copyright © 2020 Roman Gilg <subdiff@gmail.com>

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

#include "filer_controller.h"
#include "filer_helpers.h"

#include <output.h>
#include <types.h>

#include <QObject>
#include <QVariantMap>

#include <memory>

namespace Disman
{

class Output_filer
{
public:
    Output_filer(OutputPtr output, Filer_controller* controller, std::string const& dir_path)
        : m_output(output)
        , m_controller(controller)
        , m_dir_path{dir_path}
    {
        read_file();
    }

    OutputPtr output() const
    {
        return m_output;
    }

    std::string hash() const
    {
        return m_output->hash();
    }

    std::string name() const
    {
        return m_output->name();
    }

    template<typename T>
    T get_value(std::string const& id,
                T default_value,
                std::function<T(OutputPtr const&, QVariant const&, T)> getter) const
    {
        auto const val = m_info[QString::fromStdString(id)];
        return getter(m_output, val, default_value);
    }

    template<typename T>
    void set_value(std::string const& id,
                   T value,
                   std::function<void(QVariantMap&, std::string const&, T)> setter)
    {
        if (m_info.isEmpty()) {
            m_info = create_info(m_output);
        }
        setter(m_info, id, value);
    }

    static QVariantMap create_info(OutputPtr const& output)
    {
        auto metadata = [&output]() {
            QVariantMap metadata;
            metadata[QStringLiteral("name")] = QString::fromStdString(output->name());
            metadata[QStringLiteral("description")] = QString::fromStdString(output->description());
            return metadata;
        };
        QVariantMap outputInfo;
        outputInfo[QStringLiteral("id")] = QString::fromStdString(output->hash());
        outputInfo[QStringLiteral("metadata")] = metadata();
        return outputInfo;
    }

    static ModePtr get_mode(OutputPtr const& output, QVariant const& val, ModePtr default_value)
    {
        auto const val_map = val.toMap();

        bool success = true;

        auto get_resolution = [&val_map, &success]() {
            auto const key = QStringLiteral("resolution");

            if (!val_map.contains(key)) {
                success = false;
                return QSize();
            }

            auto const resolution_map = val_map[key].toMap();

            auto get_length = [&resolution_map, &success](QString axis) {
                if (!resolution_map.contains(axis)) {
                    success = false;
                    return 0.;
                }
                bool ok;
                auto const coord = resolution_map[axis].toDouble(&ok);
                success &= ok;
                return coord;
            };

            auto const width = get_length(QStringLiteral("width"));
            auto const height = get_length(QStringLiteral("height"));
            return QSize(width, height);
        };

        if (!val_map.contains(QStringLiteral("mode"))) {
            return default_value;
        }

        auto const resolution = get_resolution();

        auto get_refresh_rate = [&val_map, &success]() -> double {
            auto const key = QStringLiteral("refresh");

            if (!val_map.contains(key)) {
                success = false;
                return 0;
            }

            bool ok;
            auto const refresh = val_map[key].toDouble(&ok);
            success &= ok;
            return refresh;
        };

        auto const refresh = get_refresh_rate();
        if (!success) {
            qCWarning(DISMAN_BACKEND) << "Mode entry broken for:" << output;
            return default_value;
        }

        for (auto const& [key, mode] : output->modes()) {
            if (mode->size() == resolution && mode->refresh() == refresh) {
                return mode;
            }
        }
        return default_value;
    }

    static Output::Rotation convert_int_to_rotation(int val)
    {
        auto const rotation = static_cast<Output::Rotation>(val);
        switch (rotation) {
        case Output::Rotation::Left:
            return rotation;
        case Output::Rotation::Right:
            return rotation;
        case Output::Rotation::Inverted:
            return rotation;
        default:
            return Output::Rotation::None;
        }
    }

    QFileInfo file_info() const
    {
        return Filer_helpers::file_info(m_dir_path + "outputs/", m_output->hash());
    }

    void read_file()
    {
        Filer_helpers::read_file(file_info(), m_info);
    }

    bool write_file()
    {
        return Filer_helpers::write_file(m_info, file_info());
    }

    void get_global_data(OutputPtr& output)
    {
        auto mode = get_mode(output, m_info, nullptr);
        if (!mode) {
            return;
        }

        auto rotation = convert_int_to_rotation(Filer_helpers::from_variant(
            m_info[QStringLiteral("rotation")], static_cast<int>(Output::Rotation::None)));
        auto scale = Filer_helpers::from_variant(m_info[QStringLiteral("scale")], 1.);

        auto auto_resolution
            = Filer_helpers::from_variant(m_info[QStringLiteral("auto-resolution")], true);
        auto auto_refresh_rate
            = Filer_helpers::from_variant(m_info[QStringLiteral("auto-refresh-rate")], true);
        auto auto_rotate
            = Filer_helpers::from_variant(m_info[QStringLiteral("auto-rotate")], false);
        auto auto_rotate_only_in_tablet_mode
            = Filer_helpers::from_variant(m_info[QStringLiteral("auto-rotate-tablet-only")], false);

        output->set_global_data({mode->size(),
                                 mode->refresh(),
                                 rotation,
                                 scale,
                                 auto_resolution,
                                 auto_refresh_rate,
                                 auto_rotate,
                                 auto_rotate_only_in_tablet_mode});
    }

private:
    OutputPtr m_output;
    Filer_controller* m_controller;

    std::string m_dir_path;
    QVariantMap m_info;
};

}
