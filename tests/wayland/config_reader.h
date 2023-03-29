/*************************************************************************************
 *  Copyright 2014-2015 Sebastian Kügler <sebas@kde.org>                             *
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
#ifndef DISMAN_WAYLAND_CONFIGREADER_H
#define DISMAN_WAYLAND_CONFIGREADER_H

#include <QObject>
#include <QRect>
#include <string>

#include <Wrapland/Server/output.h>
#include <Wrapland/Server/output_manager.h>

namespace Disman
{

class WaylandConfigReader
{

public:
    static void outputsFromConfig(std::string const& configfile,
                                  Wrapland::Server::output_manager& manager,
                                  std::vector<std::unique_ptr<Wrapland::Server::output>>& outputs);
    static std::unique_ptr<Wrapland::Server::output>
    create_output(QVariantMap const& outputConfig, Wrapland::Server::output_manager& manager);

    static QSize sizeFromJson(const QVariant& data);
    static QRect rectFromJson(const QVariant& data);
    static QPoint pointFromJson(const QVariant& data);
};

}

#endif
