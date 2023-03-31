/*************************************************************************************
 *  Copyright 2015 by Sebastian Kügler <sebas@kde.org>                               *
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
#include <QObject>
#include <QSignalSpy>
#include <QtTest>

#include "backendmanager_p.h"
#include "config.h"
#include "configmonitor.h"
#include "generator.h"
#include "getconfigoperation.h"
#include "mode.h"
#include "output.h"
#include "setconfigoperation.h"

#include "server.h"

Q_LOGGING_CATEGORY(DISMAN_WAYLAND, "disman.wayland")

using namespace Disman;

class wayland_config : public QObject
{
    Q_OBJECT

public:
    explicit wayland_config(QObject* parent = nullptr);

private Q_SLOTS:
    void init();
    void cleanup();

    void changeConfig();
    void testPositionChange();
    void testRotationChange();
    void testRotationChange_data();
    void test_replication_change();
    void testModeChange();
    void test_adaptive_sync_change();
    void testApplyOnPending();

private:
    server* m_server;
};

wayland_config::wayland_config(QObject* parent)
    : QObject(parent)
    , m_server(nullptr)
{
    qputenv("DISMAN_IN_PROCESS", "1");
    qputenv("DISMAN_LOGGING", "false");
    QStandardPaths::setTestModeEnabled(true);
}

void wayland_config::init()
{
    setenv("DISMAN_BACKEND", "wayland", 1);
    Disman::BackendManager::instance()->shutdown_backend();

    // This is how KWayland will pick up the right socket,
    // and thus connect to our internal test server.
    setenv("WAYLAND_DISPLAY", s_socketName, 1);

    m_server = new server(this);
    m_server->start();
}

void wayland_config::cleanup()
{
    Disman::BackendManager::instance()->shutdown_backend();
    delete m_server;

    // Make sure we did not accidentally unset test mode prior and delete our user configuration.
    QStandardPaths::setTestModeEnabled(true);
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        % QStringLiteral("/disman/");
    QVERIFY(QDir(path).removeRecursively());
}

void wayland_config::changeConfig()
{
    auto op = new GetConfigOperation();
    QVERIFY(op->exec());
    auto config = op->config();
    QVERIFY(config);

    // Prepare monitor & spy
    Disman::ConfigMonitor* monitor = Disman::ConfigMonitor::instance();
    monitor->add_config(config);
    QSignalSpy configSpy(monitor, &Disman::ConfigMonitor::configuration_changed);

    // The first output is enabled, let's set a different mode.
    auto output = config->outputs().begin()->second;
    QVERIFY(output->enabled());
    auto mode = output->mode(QSize(1280, 960), 60000);
    QVERIFY(mode);
    output->set_mode(mode);

    auto output2 = config->outputs()[2]; // is this id stable enough?
    output2->set_position(QPoint(4000, 1080));
    output2->set_rotation(Disman::Output::Left);

    QSignalSpy serverSpy(m_server, &server::configChanged);
    auto sop = new SetConfigOperation(config, this);
    sop->exec(); // fire and forget...

    QVERIFY(configSpy.wait());
    // check if the server changed
    QCOMPARE(serverSpy.count(), 1);

    QCOMPARE(configSpy.count(), 1);

    monitor->remove_config(config);
    m_server->showOutputs();
}

void wayland_config::testPositionChange()
{
    auto op = new GetConfigOperation();
    QVERIFY(op->exec());
    auto config = op->config();
    QVERIFY(config);

    // Prepare monitor & spy
    Disman::ConfigMonitor* monitor = Disman::ConfigMonitor::instance();
    monitor->add_config(config);
    QSignalSpy configSpy(monitor, &Disman::ConfigMonitor::configuration_changed);

    auto output = config->outputs()[2]; // is this id stable enough?
    auto new_pos = QPoint(3840, 1080);
    output->set_position(new_pos);

    QSignalSpy serverSpy(m_server, &server::configChanged);
    auto sop = new SetConfigOperation(config, this);
    sop->exec(); // fire and forget...

    QVERIFY(configSpy.wait());
    // check if the server changed
    QCOMPARE(serverSpy.count(), 1);

    QCOMPARE(configSpy.count(), 1);
}

void wayland_config::testRotationChange_data()
{
    QTest::addColumn<Disman::Output::Rotation>("rotation");
    QTest::newRow("left") << Disman::Output::Left;
    QTest::newRow("inverted") << Disman::Output::Inverted;
    QTest::newRow("right") << Disman::Output::Right;
}

void wayland_config::testRotationChange()
{
    QFETCH(Disman::Output::Rotation, rotation);

    auto op = new GetConfigOperation();
    QVERIFY(op->exec());
    auto config = op->config();
    QVERIFY(config);

    // Prepare monitor & spy
    Disman::ConfigMonitor* monitor = Disman::ConfigMonitor::instance();
    monitor->add_config(config);
    QSignalSpy configSpy(monitor, &Disman::ConfigMonitor::configuration_changed);

    auto output = config->outputs().begin()->second; // is this id stable enough?
    output->set_enabled(true);
    output->set_rotation(rotation);

    QSignalSpy serverSpy(m_server, &server::configChanged);
    auto sop = new SetConfigOperation(config, this);
    sop->exec(); // fire and forget...

    QCOMPARE(configSpy.wait(500), rotation != Disman::Output::None);
    QCOMPARE(serverSpy.count(), rotation != Disman::Output::None);
    QCOMPARE(configSpy.count(), 1);

    // Get a new config, then compare the output with the expected new value
    auto newop = new GetConfigOperation();
    QVERIFY(newop->exec());
    auto newconfig = newop->config();
    QVERIFY(newconfig);

    auto newoutput = newconfig->outputs().begin()->second;
    QCOMPARE(newoutput->rotation(), rotation);
}

void wayland_config::test_replication_change()
{
    auto op = new GetConfigOperation();
    QVERIFY(op->exec());
    auto config = op->config();
    QVERIFY(config);

    auto op2 = new GetConfigOperation();
    QVERIFY(op2->exec());
    auto config2 = op2->config();
    QVERIFY(config2);

    // Prepare monitor & spy
    Disman::ConfigMonitor* monitor = Disman::ConfigMonitor::instance();
    monitor->add_config(config);
    monitor->add_config(config2);
    QSignalSpy configSpy(monitor, &Disman::ConfigMonitor::configuration_changed);
    QSignalSpy configSpy2(monitor, &Disman::ConfigMonitor::configuration_changed);

    auto output1 = config->outputs().at(1);
    auto& server_output1 = m_server->outputs.at(0);
    auto& server_output2 = m_server->outputs.at(1);
    auto output2 = config->outputs().at(2);
    QVERIFY(!output1->replication_source());
    QVERIFY(!output2->replication_source());
    QVERIFY(output1->geometry() != output2->geometry());
    QVERIFY(server_output1->get_state().geometry != server_output2->get_state().geometry);

    output2->set_replication_source(output1->id());
    QCOMPARE(output2->replication_source(), output1->id());
    QVERIFY(!config2->outputs().at(2)->replication_source());

    QSignalSpy serverSpy(m_server, &server::configChanged);
    auto sop = new SetConfigOperation(config, this);
    sop->exec(); // fire and forget...

    QVERIFY(configSpy.wait());
    QCOMPARE(serverSpy.count(), 1);
    QCOMPARE(configSpy.count(), 1);
    QCOMPARE(configSpy2.count(), 1);
    QCOMPARE(server_output1->get_state().geometry, server_output2->get_state().geometry);
    QCOMPARE(output2->replication_source(), output1->id());
    QCOMPARE(config2->outputs().at(2)->replication_source(), config2->outputs().at(1)->id());
}

void wayland_config::testModeChange()
{
    auto op = new GetConfigOperation();
    QVERIFY(op->exec());
    auto config = op->config();
    QVERIFY(config);

    Disman::ConfigMonitor* monitor = Disman::ConfigMonitor::instance();
    monitor->add_config(config);
    QSignalSpy configSpy(monitor, &Disman::ConfigMonitor::configuration_changed);

    auto output = config->outputs()[1]; // is this id stable enough?

    auto mode = output->mode(QSize(1400, 1050), 60000);
    QVERIFY(mode);
    output->set_mode(mode);
    output->set_auto_resolution(false);
    output->set_auto_refresh_rate(false);

    QSignalSpy serverSpy(m_server, &server::configChanged);
    auto sop = new SetConfigOperation(config, this);
    sop->exec();

    QVERIFY(configSpy.wait());
    // check if the server changed
    QCOMPARE(serverSpy.count(), 1);

    QCOMPARE(configSpy.count(), 1);
}

void wayland_config::test_adaptive_sync_change()
{
    auto op = new GetConfigOperation();
    QVERIFY(op->exec());
    auto config = op->config();
    QVERIFY(config);

    Disman::ConfigMonitor* monitor = Disman::ConfigMonitor::instance();
    monitor->add_config(config);
    QSignalSpy configSpy(monitor, &Disman::ConfigMonitor::configuration_changed);

    auto output = config->outputs()[1]; // is this id stable enough?

    auto enabled = output->adaptive_sync();
    output->set_adaptive_sync(!enabled);

    QSignalSpy serverSpy(m_server, &server::configChanged);
    auto sop = new SetConfigOperation(config, this);
    sop->exec();

    QVERIFY(configSpy.wait());
    QCOMPARE(serverSpy.count(), 1);
    QCOMPARE(configSpy.count(), 1);
}

void wayland_config::testApplyOnPending()
{
    auto op = new GetConfigOperation();
    QVERIFY(op->exec());
    auto config = op->config();
    QVERIFY(config);

    auto op2 = new GetConfigOperation();
    QVERIFY(op2->exec());
    auto config2 = op2->config();
    QVERIFY(config2);

    Disman::ConfigMonitor* monitor = Disman::ConfigMonitor::instance();
    monitor->add_config(config);
    monitor->add_config(config2);
    QSignalSpy configSpy(monitor, &Disman::ConfigMonitor::configuration_changed);

    auto output1 = config->outputs()[2]; // is this id stable enough?

    // Transform value in the beginning is normal.
    QCOMPARE(output1->rotation(), Output::Rotation::None);
    output1->set_rotation(Output::Rotation::Left);

    QSignalSpy serverSpy(m_server, &server::configChanged);
    QSignalSpy serverReceivedSpy(m_server, &server::configReceived);

    m_server->suspendChanges(true);

    new SetConfigOperation(config, this);

    /* Apply next config */
    auto output2 = config2->outputs()[2]; // is this id stable enough?

    QCOMPARE(output2->rotation(), Output::Rotation::None);
    output2->set_rotation(Output::Rotation::Right);

    // Will be discarded by the server.
    new SetConfigOperation(config2, this);

    QVERIFY(serverReceivedSpy.wait());
    QCOMPARE(serverReceivedSpy.count(), 1);
    m_server->suspendChanges(false);

    QVERIFY(configSpy.wait());
    QCOMPARE(serverSpy.count(), 1);
    QCOMPARE(configSpy.count(), 1);
    QCOMPARE(output1->rotation(), Output::Rotation::Left);
    QCOMPARE(output2->rotation(), Output::Rotation::Left);
}

QTEST_GUILESS_MAIN(wayland_config)

#include "wayland_config.moc"
