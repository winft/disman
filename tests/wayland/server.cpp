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
#include "server.h"

#include "config_reader.h"

#include <QLoggingCategory>
#include <Wrapland/Server/wlr_output_configuration_head_v1.h>
#include <Wrapland/Server/wlr_output_manager_v1.h>

Q_LOGGING_CATEGORY(DISMAN_WAYLAND_TESTSERVER, "disman.wayland.testserver")

using namespace Disman;

server::server(QObject* parent)
    : QObject(parent)
    , config_file_path{TEST_DATA + std::string("default.json")}
{
}

server::~server()
{
    stop();
    qCDebug(DISMAN_WAYLAND_TESTSERVER) << "Wayland server shut down.";
}

void server::start()
{
    using namespace Wrapland::Server;

    display = std::make_unique<Wrapland::Server::Display>();
    display->set_socket_name(qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")
                                 ? s_socketName
                                 : qgetenv("WAYLAND_DISPLAY").toStdString());
    display->start();

    dpms_manager = std::make_unique<Wrapland::Server::DpmsManager>(display.get());
    output_manager = std::make_unique<Wrapland::Server::output_manager>(*display);
    output_manager->create_wlr_manager_v1();

    connect(output_manager->wlr_manager_v1.get(),
            &Wrapland::Server::wlr_output_manager_v1::apply_config,
            this,
            &server::apply_config);

    config_reader::load_outputs(config_file_path, *output_manager, outputs);
    output_manager->commit_changes();

    qCDebug(DISMAN_WAYLAND_TESTSERVER)
        << QStringLiteral("export WAYLAND_DISPLAY=") + display->socket_name().c_str();
    qCDebug(DISMAN_WAYLAND_TESTSERVER) << QStringLiteral(
        "You can specify the WAYLAND_DISPLAY for this server by exporting it in the environment");
}

void server::stop()
{
    outputs.clear();
    display.reset();
}

void server::setConfig(const QString& configfile)
{
    qCDebug(DISMAN_WAYLAND_TESTSERVER) << "Creating Wayland server from " << configfile;
    config_file_path = configfile.toStdString();
}

void server::suspendChanges(bool suspend)
{
    if (pending_suspend == suspend) {
        return;
    }
    pending_suspend = suspend;
    if (!suspend && waiting_config) {
        waiting_config->send_succeeded();
        output_manager->commit_changes();
        waiting_config = nullptr;
        Q_EMIT configChanged();
    }
}

void server::apply_config(Wrapland::Server::wlr_output_configuration_v1* config)
{
    auto const& enabled_heads = config->enabled_heads();

    qCDebug(DISMAN_WAYLAND_TESTSERVER)
        << "Server received change request, heads:" << enabled_heads.size();
    Q_EMIT configReceived();

    for (auto& output : outputs) {
        auto state = output->get_state();
        auto const description = QString::fromStdString(output->get_metadata().description);

        auto it = std::find_if(enabled_heads.cbegin(), enabled_heads.cend(), [&, this](auto head) {
            return &head->get_output() == output.get();
        });
        if (it == enabled_heads.cend()) {
            qCDebug(DISMAN_WAYLAND_TESTSERVER) << "Setting disabled:" << description;
            state.enabled = false;
            output->set_state(state);
            continue;
        }

        auto const& config_state = (*it)->get_state();
        state.enabled = true;
        state.mode = config_state.mode;
        state.transform = config_state.transform;
        state.geometry = config_state.geometry;
        state.adaptive_sync = config_state.adaptive_sync;
        output->set_state(state);

        qCDebug(DISMAN_WAYLAND_TESTSERVER) << "Setting enabled:" << description;
        qCDebug(DISMAN_WAYLAND_TESTSERVER)
            << "    mode:" << state.mode.size << "@" << state.mode.refresh_rate;
        qCDebug(DISMAN_WAYLAND_TESTSERVER)
            << "    adaptive sync:" << static_cast<int>(state.adaptive_sync);
        qCDebug(DISMAN_WAYLAND_TESTSERVER) << "    transform:" << static_cast<int>(state.transform);
        qCDebug(DISMAN_WAYLAND_TESTSERVER) << "    geometry:" << state.geometry;
    }

    if (pending_suspend) {
        Q_ASSERT(!waiting_config);
        waiting_config = config;
        return;
    }

    config->send_succeeded();
    output_manager->commit_changes();
    Q_EMIT configChanged();
}

void server::showOutputs()
{
    qCDebug(DISMAN_WAYLAND_TESTSERVER)
        << "******** Wayland server running: " << outputs.size() << " outputs. ********";
    for (auto const& output : outputs) {
        auto meta = output->get_metadata();
        auto state = output->get_state();
        qCDebug(DISMAN_WAYLAND_TESTSERVER) << "  * Output: " << meta.name.c_str();
        qCDebug(DISMAN_WAYLAND_TESTSERVER)
            << "      Enabled: " << (state.enabled ? "enabled" : "disabled");
        qCDebug(DISMAN_WAYLAND_TESTSERVER)
            << "         Name: "
            << QStringLiteral("%2-%3").arg(meta.make.c_str(), meta.model.c_str());
        qCDebug(DISMAN_WAYLAND_TESTSERVER)
            << "         Mode: " << modeString(output->modes(), state.mode.id);
        qCDebug(DISMAN_WAYLAND_TESTSERVER) << "          Geo: " << state.geometry;
    }
    qCDebug(DISMAN_WAYLAND_TESTSERVER) << "******************************************************";
}

QString server::modeString(std::vector<Wrapland::Server::output_mode> const& modes, int mid)
{
    QString s;
    QString ids;
    int _i = 0;

    for (auto const& mode : modes) {
        _i++;
        if (_i < 6) {
            ids.append(QString::number(mode.id) + QLatin1String(", "));
        } else {
            ids.append(QLatin1Char('.'));
        }
        if (mode.id == mid) {
            s = QStringLiteral("%1x%2 @%3")
                    .arg(QString::number(mode.size.width()),
                         QString::number(mode.size.height()),
                         QString::number(mode.refresh_rate));
        }
    }
    return QStringLiteral("[%1] %2 (%4 modes: %3)")
        .arg(QString::number(mid), s, ids, QString::number(modes.size()));
}
