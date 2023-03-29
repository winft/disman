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
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QObject>
#include <QtTest>

#include "backendmanager_p.h"
#include "config.h"
#include "configmonitor.h"
#include "getconfigoperation.h"
#include "mode.h"
#include "output.h"
#include "setconfigoperation.h"

#include "server.h"

Q_LOGGING_CATEGORY(DISMAN_WAYLAND, "disman.wayland")

using namespace Disman;

class wayland_backend : public QObject
{
    Q_OBJECT

public:
    explicit wayland_backend(QObject* parent = nullptr);

private Q_SLOTS:
    void init();
    void cleanup();
    void loadConfig();

    void verifyConfig();
    void verifyScreen();
    void verifyOutputs();
    void verifyModes();
    void verifyIds();
    void verifyFeatures();
    void simpleWrite();
    void addAndRemoveOutput();

private:
    ConfigPtr m_config;
    server* m_server;
};

wayland_backend::wayland_backend(QObject* parent)
    : QObject(parent)
    , m_config(nullptr)
{
    qputenv("DISMAN_IN_PROCESS", "1");
    qputenv("DISMAN_LOGGING", "false");
    QStandardPaths::setTestModeEnabled(true);

    m_server = new server(this);
    m_server->setConfig(QLatin1String(TEST_DATA) + QLatin1String("multipleoutput.json"));
}

void wayland_backend::init()
{
    qputenv("DISMAN_BACKEND", "wayland");

    // This is how Wrapland will pick up the right socket,
    // and thus connect to our internal test server.
    setenv("WAYLAND_DISPLAY", s_socketName, 1);
    m_server->start();

    GetConfigOperation* op = new GetConfigOperation();
    op->exec();
    m_config = op->config();
    QVERIFY(m_config);
}

void wayland_backend::cleanup()
{
    Disman::BackendManager::instance()->shutdown_backend();
    m_server->stop();

    // Make sure we did not accidentally unset test mode prior and delete our user configuration.
    QStandardPaths::setTestModeEnabled(true);
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        % QStringLiteral("/disman/");
    QVERIFY(QDir(path).removeRecursively());
}

void wayland_backend::loadConfig()
{
    GetConfigOperation* op = new GetConfigOperation();
    op->exec();
    m_config = op->config();
    QVERIFY(m_config->valid());
    qCDebug(DISMAN_WAYLAND) << "Outputs:" << m_config->outputs();
}

void wayland_backend::verifyConfig()
{
    QVERIFY(m_config != nullptr);
}

void wayland_backend::verifyScreen()
{
    ScreenPtr screen = m_config->screen();

    QVERIFY(screen->min_size().width() <= screen->max_size().width());
    QVERIFY(screen->min_size().height() <= screen->max_size().height());

    QVERIFY(screen->min_size().width() <= screen->current_size().width());
    QVERIFY(screen->min_size().height() <= screen->current_size().height());

    QVERIFY(screen->max_size().width() >= screen->current_size().width());
    QVERIFY(screen->max_size().height() >= screen->current_size().height());
    QVERIFY(m_config->screen()->max_outputs_count() > 0);
}

void wayland_backend::verifyOutputs()
{
    QVERIFY(!m_config->primary_output());
    QVERIFY(m_config->outputs().size());
    QCOMPARE(m_server->outputs.size(), m_config->outputs().size());

    QList<int> ids;
    for (auto const& [key, output] : m_config->outputs()) {
        QVERIFY(output->name().size());
        QVERIFY(output->id() > -1);
        QVERIFY(output->geometry() != QRectF(1, 1, 1, 1));
        QVERIFY(output->geometry() != QRectF());
        QVERIFY(output->physical_size() != QSize());
        QVERIFY(output->preferred_modes().size() == 1);
        QCOMPARE(output->rotation(), Output::None);
        QVERIFY(!ids.contains(output->id()));
        ids << output->id();
    }
}

void wayland_backend::verifyModes()
{
    for (auto const& [key, output] : m_config->outputs()) {
        for (auto const& [key, mode] : output->modes()) {
            QVERIFY(!mode->name().empty());
            QVERIFY(mode->refresh() > 0);
            QVERIFY(mode->size().isValid());
        }
    }
}

void wayland_backend::verifyIds()
{
    QList<quint32> ids;
    for (auto const& [key, output] : m_config->outputs()) {
        QVERIFY(ids.contains(output->id()) == false);
        QVERIFY(output->id() > 0);
        ids << output->id();
    }
}

void wayland_backend::simpleWrite()
{
    Disman::BackendManager::instance()->shutdown_backend();
    GetConfigOperation* op = new GetConfigOperation();
    op->exec();
    m_config = op->config();
    auto output = m_config->output(2);
    QVERIFY(output);
    auto mode = output->mode("4");
    QVERIFY(mode);
    output->set_mode(mode);
    QCOMPARE(output->commanded_mode()->size(), QSize(800, 600));

    auto setop = new SetConfigOperation(m_config);
    QVERIFY(setop->exec());
}

void wayland_backend::addAndRemoveOutput()
{
    Disman::BackendManager::instance()->shutdown_backend();
    GetConfigOperation* op = new GetConfigOperation();
    op->exec();
    auto config = op->config();
    QCOMPARE(config->outputs().size(), 2);
    Disman::ConfigMonitor* monitor = Disman::ConfigMonitor::instance();
    monitor->add_config(config);
    QSignalSpy configSpy(monitor, &Disman::ConfigMonitor::configuration_changed);

    // Now add an outputdevice on the server side
    Wrapland::Server::output_metadata output_meta;
    output_meta.name = "1337";
    m_server->outputs.emplace_back(
        std::make_unique<Wrapland::Server::output>(output_meta, *m_server->output_manager));

    Wrapland::Server::output_mode m0;
    m0.id = 0;
    m0.size = QSize(800, 600);
    m0.preferred = true;
    m_server->outputs.back()->add_mode(m0);

    Wrapland::Server::output_mode m2;
    m2.id = 2;
    m2.size = QSize(1280, 1024);
    m2.refresh_rate = 90000;
    m_server->outputs.back()->add_mode(m2);

    Wrapland::Server::output_mode m1;
    m1.id = 1;
    m1.size = QSize(1024, 768);
    m_server->outputs.back()->add_mode(m1);

    auto state = m_server->outputs.back()->get_state();
    state.geometry = {QPoint(), m1.size};
    m_server->outputs.back()->set_state(state);
    m_server->output_manager->commit_changes();

    QVERIFY(configSpy.wait());
    // QTRY_VERIFY(configSpy.count());

    GetConfigOperation* op2 = new GetConfigOperation();
    op2->exec();
    auto newconfig = op2->config();
    QCOMPARE(newconfig->outputs().size(), 3);

    // Now remove the output again.
    m_server->outputs.pop_back();
    QVERIFY(configSpy.wait());
    GetConfigOperation* op3 = new GetConfigOperation();
    op3->exec();
    newconfig = op3->config();
    QCOMPARE(newconfig->outputs().size(), 2);
}

void wayland_backend::verifyFeatures()
{
    GetConfigOperation* op = new GetConfigOperation();
    op->exec();
    auto config = op->config();
    QVERIFY(!config->supported_features().testFlag(Config::Feature::None));
    QVERIFY(config->supported_features().testFlag(Config::Feature::Writable));
    QVERIFY(!config->supported_features().testFlag(Config::Feature::PrimaryDisplay));
}

QTEST_GUILESS_MAIN(wayland_backend)

#include "wayland_backend.moc"
