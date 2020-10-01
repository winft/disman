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
#include "output_filer.h"

#include "logging.h"
#include <config.h>
#include <output.h>
#include <types.h>

#include <QObject>
#include <QStandardPaths>
#include <QVariantMap>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace Disman
{

class Filer
{
public:
    Filer(Disman::ConfigPtr const& config, Filer_controller* controller, std::string suffix = "")
        : m_config{config}
        , m_controller{controller}
        , m_suffix{suffix}
    {
        m_dir_path = QString(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                             % QStringLiteral("/disman/control/"))
                         .toStdString();
        m_read_success = read_file();

        for (auto const& [key, output] : config->outputs()) {
            m_output_filers.push_back(
                std::unique_ptr<Output_filer>(new Output_filer(output, m_controller, m_dir_path)));
        }
    }

    bool get_values(ConfigPtr& config)
    {
        auto outputs = config->outputs();

        for (auto& [key, output] : outputs) {
            //
            // First we must set the retention for all later calls to get_value().
            auto retention = get_value(
                output, "retention", static_cast<int>(Output::Retention::Undefined), nullptr);
            output->set_retention(convert_int_to_retention(retention));
            output->set_enabled(get_value(output, "enabled", true, nullptr));

            if (config->supported_features().testFlag(Disman::Config::Feature::PrimaryDisplay)) {
                if (get_value(output, "primary", false, nullptr)) {
                    config->set_primary_output(output);
                }
            }

            output->set_position(
                get_value(output, "pos", QPointF(0, 0), nullptr, std::function{get_pos}));
            get_replication_source(output, outputs);

            auto filer = get_output_filer(output);
            assert(filer);

            filer->get_global_data(output);

            if (auto mode = get_value(
                    output, "mode", ModePtr(), filer, std::function{Output_filer::get_mode})) {
                output->set_mode(mode);
            } else {
                // Set an invalid commanded mode.
                output->set_resolution(QSize());
                output->set_refresh_rate(0);
            }

            if (config->supported_features().testFlag(Disman::Config::Feature::PerOutputScaling)) {
                output->set_scale(get_value(output, "scale", 1., filer));
            }

            auto const rotation
                = get_value(output, "rotation", static_cast<int>(Output::Rotation::None), filer);
            output->set_rotation(Output_filer::convert_int_to_rotation(rotation));

            output->set_auto_resolution(get_value(output, "auto-resolution", true, filer));
            output->set_auto_refresh_rate(get_value(output, "auto-refresh-rate", true, filer));

            if (config->supported_features().testFlag(Disman::Config::Feature::AutoRotation)) {
                output->set_auto_rotate(get_value(output, "auto-rotate", false, filer));
                if (config->supported_features().testFlag(Disman::Config::Feature::TabletMode)) {
                    output->set_auto_rotate_only_in_tablet_mode(
                        get_value(output, "auto-rotate-tablet-only", false, filer));
                }
            }
        }

        return m_read_success;
    }

    void set_values(ConfigPtr const& config)
    {
        auto outputs = config->outputs();
        for (auto const& [key, output] : outputs) {
            auto const retention = output->retention();
            Output_filer* filer = nullptr;

            if (retention != Output::Retention::Individual) {
                filer = get_output_filer(output);
                assert(filer);
            }

            set_value(output, "retention", static_cast<int>(output->retention()), nullptr);
            set_value(output, "enabled", output->enabled(), nullptr);

            if (config->supported_features().testFlag(Disman::Config::Feature::PrimaryDisplay)) {
                set_value(output, "primary", config->primary_output().get() == output.get(), filer);
            }

            set_replication_source(output, config);
            set_value(output, "pos", output->position(), nullptr, std::function{set_pos});

            set_value(output, "mode", output->auto_mode(), filer, std::function{set_mode});

            if (config->supported_features().testFlag(Disman::Config::Feature::PerOutputScaling)) {
                set_value(output, "scale", output->scale(), filer);
            }
            set_value(output, "rotation", static_cast<int>(output->rotation()), filer);

            set_value(output, "auto-resolution", output->auto_resolution(), filer);
            set_value(output, "auto-refresh-rate", output->auto_refresh_rate(), filer);

            if (config->supported_features().testFlag(Disman::Config::Feature::AutoRotation)) {
                set_value(output, "auto-rotate", output->auto_rotate(), filer);
                if (config->supported_features().testFlag(Disman::Config::Feature::TabletMode)) {
                    set_value(output,
                              "auto-rotate-tablet-only",
                              output->auto_rotate_only_in_tablet_mode(),
                              filer);
                }
            }
        }
    }

    template<typename T>
    T get_value(
        OutputPtr const& output,
        std::string const& id,
        T default_value,
        Output_filer* filer,
        std::function<T(OutputPtr const&, QVariant const&, T)> getter
        = []([[maybe_unused]] OutputPtr const& output, QVariant const& val, T default_value) {
              return Filer_helpers::from_variant(val, default_value);
          }) const
    {
        if (!filer || output->retention() == Output::Retention::Individual) {
            auto const outputs_info = get_outputs_info();

            for (auto const& variant_info : outputs_info) {
                auto const info = variant_info.toMap();
                if (is_info_for_output(info, output)) {
                    auto const val = info[QString::fromStdString(id)];
                    return getter(output, val, default_value);
                }
            }
        }

        // Retention is global or info for output not in config control file. If an output filer
        // was provided we hand over to that, otherwise we can only return the default value.
        if (filer) {
            return filer->get_value(id, default_value, getter);
        }
        return default_value;
    }

    template<typename T>
    void set_value(
        OutputPtr const& output,
        std::string const& id,
        T value,
        Output_filer* filer,
        std::function<void(QVariantMap&, std::string const&, T)> setter
        = [](QVariantMap& info, std::string const& id, T value) {
              info[QString::fromStdString(id)] = value;
          })
    {
        QList<QVariant>::iterator it;
        auto outputs_info = get_outputs_info();

        auto set_output_value = [&]() {
            if (filer) {
                filer->set_value(id, value, setter);
            }
        };

        for (it = outputs_info.begin(); it != outputs_info.end(); ++it) {
            auto output_info = (*it).toMap();
            if (!is_info_for_output(output_info, output)) {
                continue;
            }
            setter(output_info, id, value);
            *it = output_info;
            set_outputs(outputs_info);
            set_output_value();
            return;
        }

        // No entry yet, create one.
        auto output_info = Output_filer::create_info(output);
        setter(output_info, id, value);

        outputs_info << output_info;
        set_outputs(outputs_info);
        set_output_value();
    }

    static QPointF
    get_pos([[maybe_unused]] OutputPtr const& output, QVariant const& val, QPointF default_value)
    {
        auto const val_map = val.toMap();

        bool success = true;
        auto get_coordinate = [&val_map, &success](QString axis) {
            if (!val_map.contains(axis)) {
                success = false;
                return 0.;
            }
            bool ok;
            auto const coord = val_map[axis].toDouble(&ok);
            success &= ok;
            return coord;
        };

        auto const x = get_coordinate(QStringLiteral("x"));
        auto const y = get_coordinate(QStringLiteral("y"));

        return success ? QPointF(x, y) : default_value;
    }

    static void set_pos(QVariantMap& info, [[maybe_unused]] std::string const& id, QPointF pos)
    {
        assert(id == "pos");

        auto pos_info = [&pos]() {
            QVariantMap info;
            info[QStringLiteral("x")] = pos.x();
            info[QStringLiteral("y")] = pos.y();
            return info;
        };

        info[QStringLiteral("pos")] = pos_info();
    }

    static void
    set_mode(QVariantMap& info, [[maybe_unused]] std::string const& id, Disman::ModePtr mode)
    {
        assert(id == "mode");

        auto size_info = [&mode]() {
            QVariantMap info;
            info[QStringLiteral("width")] = mode->size().width();
            info[QStringLiteral("height")] = mode->size().height();
            return info;
        };

        auto mode_info = [&mode, size_info]() {
            QVariantMap info;
            info[QStringLiteral("refresh")] = mode->refresh();
            info[QStringLiteral("resolution")] = size_info();

            return info;
        };

        info[QStringLiteral("mode")] = mode_info();
    }

    void get_replication_source(OutputPtr& output, OutputMap const& outputs) const
    {
        auto replicate_hash = get_value(output, "replicate", QString(), nullptr).toStdString();

        auto replication_source_it
            = std::find_if(outputs.cbegin(), outputs.cend(), [replicate_hash](auto const& out) {
                  return out.second->hash() == replicate_hash;
              });

        if (replication_source_it != outputs.cend()) {
            if (replicate_hash == output->hash()) {
                qCWarning(DISMAN_BACKEND) << "Control file has sets" << output
                                          << "as its own replica. This is not allowed.";
            } else {
                output->set_replication_source(replication_source_it->first);
            }
        } else {
            output->set_replication_source(0);
        }
    }

    void set_replication_source(OutputPtr const& output, ConfigPtr const& config)
    {
        auto const source = config->output(output->replication_source());
        set_value(output,
                  "replicate",
                  source ? QString::fromStdString(source->hash()) : QStringLiteral(""),
                  nullptr);
    }

    std::string dir_path() const
    {
        return m_dir_path + "configs/";
    }

    std::string file_name() const
    {
        auto file_name = m_config->hash().toStdString();
        if (!m_suffix.empty()) {
            file_name += "-" + m_suffix;
        }
        return file_name;
    }

    QFileInfo file_info() const
    {
        return Filer_helpers::file_info(dir_path(), file_name());
    }

    bool read_file()
    {
        return Filer_helpers::read_file(file_info(), m_info);
    }

    bool write(ConfigPtr const& config)
    {
        set_values(config);

        bool success = true;

        for (auto& output_filer : m_output_filers) {
            auto const output = config->output(output_filer->output()->id());
            if (!output) {
                // TODO: fallback or reverse clean up?
                qCDebug(DISMAN_BACKEND)
                    << "Could not identify output filer" << output_filer->output()->name().c_str();
                continue;
            }

            if (output->retention() == Output::Retention::Individual) {
                continue;
            }
            success &= output_filer->write_file();
        }

        success &= Filer_helpers::write_file(m_info, file_info());
        return success;
    }

    static Output::Retention convert_int_to_retention(int val)
    {
        if (val == static_cast<int>(Output::Retention::Global)) {
            return Output::Retention::Global;
        }
        if (val == static_cast<int>(Output::Retention::Individual)) {
            return Output::Retention::Individual;
        }
        return Output::Retention::Undefined;
    }

    ConfigPtr config() const
    {
        return m_config;
    }

private:
    QVariantList get_outputs_info() const
    {
        return m_info[QStringLiteral("outputs")].toList();
    }

    void set_outputs(QVariantList outputsInfo)
    {
        m_info[QStringLiteral("outputs")] = outputsInfo;
    }

    bool is_info_for_output(QVariantMap const& info, OutputPtr const& output) const
    {
        auto const output_id_info = info[QStringLiteral("id")].toString().toStdString();

        if (!output_id_info.size()) {
            return false;
        }
        if (output->hash() != output_id_info) {
            return false;
        }

        return true;
    }

    Output_filer* get_output_filer(OutputPtr const& output) const
    {
        for (auto& filer : m_output_filers) {
            if (filer->hash() == output->hash() && filer->name() == output->name()) {
                return filer.get();
            }
        }
        return nullptr;
    }

    ConfigPtr m_config;
    Filer_controller* m_controller;

    std::vector<std::unique_ptr<Output_filer>> m_output_filers;

    std::string m_dir_path;
    std::string m_suffix;

    QVariantMap m_info;
    bool m_read_success{false};
};

}
