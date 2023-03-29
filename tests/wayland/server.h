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
#ifndef DISMAN_WAYLAND_TESTSERVER_H
#define DISMAN_WAYLAND_TESTSERVER_H

#include <QObject>

#include <Wrapland/Server/compositor.h>
#include <Wrapland/Server/display.h>
#include <Wrapland/Server/dpms.h>
#include <Wrapland/Server/output_manager.h>
#include <Wrapland/Server/wlr_output_configuration_v1.h>

namespace Disman
{

static constexpr auto s_socketName = "disman-test-wayland-backend-0";

class server : public QObject
{
    Q_OBJECT

public:
    explicit server(QObject* parent = nullptr);
    ~server() override;

    void setConfig(const QString& configfile);
    void start();
    void stop();
    void pickupConfigFile(const QString& configfile);

    void showOutputs();
    void suspendChanges(bool suspend);

    std::unique_ptr<Wrapland::Server::Display> display;
    std::vector<std::unique_ptr<Wrapland::Server::output>> outputs;
    std::unique_ptr<Wrapland::Server::output_manager> output_manager;
    std::unique_ptr<Wrapland::Server::DpmsManager> dpms_manager;

Q_SIGNALS:
    void outputsChanged();

    void started();

    void configReceived();
    void configChanged();

private Q_SLOTS:

private:
    void apply_config(Wrapland::Server::wlr_output_configuration_v1* config);
    static QString modeString(std::vector<Wrapland::Server::output_mode> const& modes, int mid);

    std::string config_file_path;
    bool pending_suspend{false};
    Wrapland::Server::wlr_output_configuration_v1* waiting_config{nullptr};
};

}

#endif
