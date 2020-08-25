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

#include <edid.h>
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

    QString hash() const
    {
        return m_output->hash();
    }

    QString name() const
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
            metadata[QStringLiteral("name")] = output->name();
            if (output->edid() && output->edid()->isValid()) {
                metadata[QStringLiteral("edid-name")]
                    = QString::fromStdString(output->edid()->deviceId());
            }
            return metadata;
        };
        QVariantMap outputInfo;
        outputInfo[QStringLiteral("id")] = output->hash();
        outputInfo[QStringLiteral("metadata")] = metadata();
        return outputInfo;
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

private:
    OutputPtr m_output;
    Filer_controller* m_controller;

    std::string m_dir_path;
    QVariantMap m_info;
};

}
