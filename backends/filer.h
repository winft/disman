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

#include "../logging.h"
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
    Filer(Disman::ConfigPtr const& config, Filer_controller* controller)
        : m_config{config}
        , m_controller{controller}
    {
        m_dir_path = QString(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                             % QStringLiteral("/disman/control/"))
                         .toStdString();
        read_file();

        // As global outputs are indexed by a hash of their edid, which is not unique,
        // to be able to tell apart multiple identical outputs, these need special treatment
        QStringList allIds;
        const auto outputs = config->outputs();
        allIds.reserve(outputs.count());
        for (const Disman::OutputPtr& output : outputs) {
            const auto outputId = output->hash();
            if (allIds.contains(outputId) && !m_duplicateOutputIds.contains(outputId)) {
                m_duplicateOutputIds << outputId;
            }
            allIds << outputId;
        }

        for (auto output : outputs) {
            m_output_filers.push_back(
                std::unique_ptr<Output_filer>(new Output_filer(output, m_controller, m_dir_path)));
        }
    }

    void get_values(ConfigPtr& config)
    {
        auto outputs = config->outputs();

        for (auto& output : outputs) {
            //
            // First we must set the retention for all later calls to get_value().
            auto retention = get_value(
                output, "retention", static_cast<int>(Output::Retention::Undefined), nullptr);
            output->set_retention(convert_int_to_retention(retention));

            get_replication_source(output, outputs);

            auto filer = get_output_filer(output);
            assert(filer);

            if (config->supportedFeatures().testFlag(Disman::Config::Feature::PerOutputScaling)) {
                output->setScale(get_value(output, "scale", 1., filer));
            }

            output->set_auto_resolution(get_value(output, "auto-resolution", true, filer));
            output->set_auto_refresh_rate(get_value(output, "auto-refresh-rate", true, filer));

            if (config->supportedFeatures().testFlag(Disman::Config::Feature::AutoRotation)) {
                output->set_auto_rotate(get_value(output, "auto-rotate", false, filer));
                if (config->supportedFeatures().testFlag(Disman::Config::Feature::TabletMode)) {
                    output->set_auto_rotate_only_in_tablet_mode(
                        get_value(output, "auto-rotate-tablet-only", false, filer));
                }
            }
        }
    }

    void set_values(ConfigPtr const& config)
    {
        auto outputs = config->outputs();
        for (auto output : outputs) {
            auto const retention = output->retention();
            Output_filer* filer = nullptr;

            if (retention != Output::Retention::Individual) {
                filer = get_output_filer(output);
                assert(filer);
            }

            set_value(output, "retention", static_cast<int>(output->retention()), nullptr);

            set_replication_source(output, config);

            if (config->supportedFeatures().testFlag(Disman::Config::Feature::PerOutputScaling)) {
                set_value(output, "scale", output->scale(), filer);
            }

            set_value(output, "auto-resolution", output->auto_resolution(), filer);
            set_value(output, "auto-refresh-rate", output->auto_refresh_rate(), filer);

            if (config->supportedFeatures().testFlag(Disman::Config::Feature::AutoRotation)) {
                set_value(output, "auto-rotate", output->auto_rotate(), filer);
                if (config->supportedFeatures().testFlag(Disman::Config::Feature::TabletMode)) {
                    set_value(output,
                              "auto-rotate-tablet-only",
                              output->auto_rotate_only_in_tablet_mode(),
                              filer);
                }
            }
        }
    }

    template<typename T>
    T get_value(OutputPtr const& output,
                std::string const& id,
                T default_value,
                Output_filer* filer) const
    {
        if (!filer || output->retention() == Output::Retention::Individual) {
            auto const outputs_info = get_outputs_info();

            for (auto const& variant_info : outputs_info) {
                auto const info = variant_info.toMap();
                if (is_info_for_output(info, output)) {
                    auto const val = info[QString::fromStdString(id)];
                    return Filer_helpers::from_variant(val, default_value);
                }
            }
        }

        // Retention is global or info for output not in config control file. If an output filer
        // was provided we hand over to that, otherwise we can only return the default value.
        if (filer) {
            return filer->get_value(id, default_value);
        }
        return default_value;
    }

    template<typename T>
    void set_value(OutputPtr const& output, std::string const& id, T value, Output_filer* filer)
    {
        QList<QVariant>::iterator it;
        auto outputs_info = get_outputs_info();

        auto set_output_value = [filer, &id, value]() {
            if (filer) {
                filer->set_value(id, value);
            }
        };

        for (it = outputs_info.begin(); it != outputs_info.end(); ++it) {
            auto output_info = (*it).toMap();
            if (!is_info_for_output(output_info, output)) {
                continue;
            }
            output_info[QString::fromStdString(id)] = value;
            *it = output_info;
            set_outputs(outputs_info);
            set_output_value();
            return;
        }

        // No entry yet, create one.
        auto output_info = Output_filer::create_info(output);
        output_info[QString::fromStdString(id)] = value;

        outputs_info << output_info;
        set_outputs(outputs_info);
        set_output_value();
    }

    void get_replication_source(OutputPtr& output, OutputList const& outputs) const
    {
        auto replicate_hash = get_value(output, "replicate", QString(), nullptr);

        auto replication_source_it = std::find_if(
            outputs.constBegin(), outputs.constEnd(), [replicate_hash](OutputPtr const& out) {
                return out->hash() == replicate_hash;
            });

        if (replication_source_it != outputs.constEnd()) {
            if (replicate_hash == output->hash()) {
                qCWarning(DISMAN_BACKEND) << "Control file has sets" << output
                                          << "as its own replica. This is not allowed.";
            } else {
                output->setReplicationSource((*replication_source_it)->id());
            }
        } else {
            output->setReplicationSource(0);
        }
    }

    void set_replication_source(OutputPtr const& output, ConfigPtr const& config)
    {
        auto const source = config->output(output->replicationSource());
        set_value(output, "replicate", source ? source->hash() : QStringLiteral(""), nullptr);
    }

    QFileInfo file_info() const
    {
        return Filer_helpers::file_info(m_dir_path + "configs/", m_config->connectedOutputsHash());
    }

    void read_file()
    {
        Filer_helpers::read_file(file_info(), m_info);
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
                    << "Could not identify output filer" << output_filer->output()->name();
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
        auto const output_id_info = info[QStringLiteral("id")].toString();

        if (output_id_info.isEmpty()) {
            return false;
        }
        if (output->hash() != output_id_info) {
            return false;
        }

        if (!output->name().isEmpty()
            && m_duplicateOutputIds.contains(QString::number(output->id()))) {
            // We may have identical outputs connected, these will have the same id in the config
            // in order to find the right one, also check the output's name (usually the connector)
            auto const metadata = info[QStringLiteral("metadata")].toMap();
            auto const outputNameInfo = metadata[QStringLiteral("name")].toString();
            if (output->name() != outputNameInfo) {
                // Was a duplicate id, but info not for this output.
                return false;
            }
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
    QStringList m_duplicateOutputIds;

    std::string m_dir_path;
    QVariantMap m_info;
};

}
