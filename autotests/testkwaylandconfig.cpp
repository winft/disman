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
#include <QtTest>
#include <QObject>
#include <QSignalSpy>

#include "backendmanager_p.h"
#include "getconfigoperation.h"
#include "setconfigoperation.h"
#include "config.h"
#include "configmonitor.h"
#include "output.h"
#include "mode.h"
#include "edid.h"

#include "waylandtestserver.h"

Q_LOGGING_CATEGORY(DISMAN_WAYLAND, "disman.wayland.kwayland")

using namespace Disman;

class TestKWaylandConfig : public QObject
{
    Q_OBJECT

public:
    explicit TestKWaylandConfig(QObject *parent = nullptr);

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void changeConfig();
    void testPositionChange();
    void testRotationChange();
    void testRotationChange_data();
    void testScaleChange();
    void testModeChange();
    void testApplyOnPending();

private:

    WaylandTestServer *m_server;

};

TestKWaylandConfig::TestKWaylandConfig(QObject *parent)
    : QObject(parent)
    , m_server(nullptr)
{
    qputenv("DISMAN_BACKEND_INPROCESS", "1");
    qputenv("DISMAN_LOGGING", "false");
}

void TestKWaylandConfig::initTestCase()
{
    setenv("DISMAN_BACKEND", "wayland", 1);
    Disman::BackendManager::instance()->shutdownBackend();

    // This is how KWayland will pick up the right socket,
    // and thus connect to our internal test server.
    setenv("WAYLAND_DISPLAY", s_socketName.toLocal8Bit().constData(), 1);

    m_server = new WaylandTestServer(this);
    m_server->start();
}

void TestKWaylandConfig::cleanupTestCase()
{
    qDebug() << "Shutting down";
    Disman::BackendManager::instance()->shutdownBackend();
    delete m_server;
}

void TestKWaylandConfig::changeConfig()
{
    auto op = new GetConfigOperation();
    QVERIFY(op->exec());
    auto config = op->config();
    QVERIFY(config);

    // Prepare monitor & spy
    Disman::ConfigMonitor *monitor = Disman::ConfigMonitor::instance();
    monitor->addConfig(config);
    QSignalSpy configSpy(monitor, &Disman::ConfigMonitor::configurationChanged);


    // The first output is currently disabled, let's try to enable it
    auto output = config->outputs().first();
    QVERIFY(output->isEnabled() == false);
    output->setEnabled(true);
    output->setCurrentModeId(QStringLiteral("76"));

    auto output2 = config->outputs()[2]; // is this id stable enough?
    output2->setPosition(QPoint(4000, 1080));
    output2->setRotation(Disman::Output::Left);

    QSignalSpy serverSpy(m_server, &WaylandTestServer::configChanged);
    auto sop = new SetConfigOperation(config, this);
    sop->exec(); // fire and forget...

    QVERIFY(configSpy.wait());
    // check if the server changed
    QCOMPARE(serverSpy.count(), 1);

    QCOMPARE(configSpy.count(), 1);

    monitor->removeConfig(config);
    m_server->showOutputs();
}

void TestKWaylandConfig::testPositionChange()
{
    auto op = new GetConfigOperation();
    QVERIFY(op->exec());
    auto config = op->config();
    QVERIFY(config);

    // Prepare monitor & spy
    Disman::ConfigMonitor *monitor = Disman::ConfigMonitor::instance();
    monitor->addConfig(config);
    QSignalSpy configSpy(monitor, &Disman::ConfigMonitor::configurationChanged);

    auto output = config->outputs()[2]; // is this id stable enough?
    auto new_pos = QPoint(3840, 1080);
    output->setPosition(new_pos);

    QSignalSpy serverSpy(m_server, &WaylandTestServer::configChanged);
    auto sop = new SetConfigOperation(config, this);
    sop->exec(); // fire and forget...

    QVERIFY(configSpy.wait());
    // check if the server changed
    QCOMPARE(serverSpy.count(), 1);

    QCOMPARE(configSpy.count(), 1);
}

void TestKWaylandConfig::testRotationChange_data()
{
    QTest::addColumn<Disman::Output::Rotation>("rotation");
    QTest::newRow("left") << Disman::Output::Left;
    QTest::newRow("inverted") << Disman::Output::Inverted;
    QTest::newRow("right") << Disman::Output::Right;
    QTest::newRow("none") << Disman::Output::None;
}

void TestKWaylandConfig::testRotationChange()
{
    QFETCH(Disman::Output::Rotation, rotation);

    auto op = new GetConfigOperation();
    QVERIFY(op->exec());
    auto config = op->config();
    QVERIFY(config);

    // Prepare monitor & spy
    Disman::ConfigMonitor *monitor = Disman::ConfigMonitor::instance();
    monitor->addConfig(config);
    QSignalSpy configSpy(monitor, &Disman::ConfigMonitor::configurationChanged);

    auto output = config->outputs().first(); // is this id stable enough?
    output->setRotation(rotation);

    QSignalSpy serverSpy(m_server, &WaylandTestServer::configChanged);
    auto sop = new SetConfigOperation(config, this);
    sop->exec(); // fire and forget...

    QVERIFY(configSpy.wait());
    // check if the server changed
    QCOMPARE(serverSpy.count(), 1);

    QCOMPARE(configSpy.count(), 1);

    // Get a new config, then compare the output with the expected new value
    auto newop = new GetConfigOperation();
    QVERIFY(newop->exec());
    auto newconfig = newop->config();
    QVERIFY(newconfig);

    auto newoutput = newconfig->outputs().first();
    QCOMPARE(newoutput->rotation(), rotation);

}

void TestKWaylandConfig::testScaleChange()
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
    Disman::ConfigMonitor *monitor = Disman::ConfigMonitor::instance();
    monitor->addConfig(config);
    monitor->addConfig(config2);
    QSignalSpy configSpy(monitor, &Disman::ConfigMonitor::configurationChanged);
    QSignalSpy configSpy2(monitor, &Disman::ConfigMonitor::configurationChanged);

    auto output2 = config2->outputs()[2]; // is this id stable enough?
    QSignalSpy outputSpy(output2.data(), &Disman::Output::geometryChanged);
    QCOMPARE(output2->scale(), 1.0);

    auto output = config->outputs()[2]; // is this id stable enough?
    output->setScale(2);

    QSignalSpy serverSpy(m_server, &WaylandTestServer::configChanged);
    auto sop = new SetConfigOperation(config, this);
    sop->exec(); // fire and forget...

    QVERIFY(configSpy.wait());
    // check if the server changed
    QCOMPARE(serverSpy.count(), 1);

    QCOMPARE(configSpy.count(), 1);
    QCOMPARE(outputSpy.count(), 1);
    QCOMPARE(configSpy2.count(), 1);
    QCOMPARE(output2->scale(), 2.0);
}

void TestKWaylandConfig::testModeChange()
{
    auto op = new GetConfigOperation();
    QVERIFY(op->exec());
    auto config = op->config();
    QVERIFY(config);

    Disman::ConfigMonitor *monitor = Disman::ConfigMonitor::instance();
    monitor->addConfig(config);
    QSignalSpy configSpy(monitor, &Disman::ConfigMonitor::configurationChanged);

    auto output = config->outputs()[1]; // is this id stable enough?

    QString new_mode = QStringLiteral("74");
    output->setCurrentModeId(new_mode);

    QSignalSpy serverSpy(m_server, &WaylandTestServer::configChanged);
    auto sop = new SetConfigOperation(config, this);
    sop->exec();

    QVERIFY(configSpy.wait());
    // check if the server changed
    QCOMPARE(serverSpy.count(), 1);

    QCOMPARE(configSpy.count(), 1);
}

void TestKWaylandConfig::testApplyOnPending()
{
    auto op = new GetConfigOperation();
    QVERIFY(op->exec());
    auto config = op->config();
    QVERIFY(config);

    auto op2 = new GetConfigOperation();
    QVERIFY(op2->exec());
    auto config2 = op2->config();
    QVERIFY(config2);

    Disman::ConfigMonitor *monitor = Disman::ConfigMonitor::instance();
    monitor->addConfig(config);
    QSignalSpy configSpy(monitor, &Disman::ConfigMonitor::configurationChanged);

    auto output = config->outputs()[1]; // is this id stable enough?

    QCOMPARE(output->scale(), 1.0);
    output->setScale(2);

    QSignalSpy serverSpy(m_server, &WaylandTestServer::configChanged);
    QSignalSpy serverReceivedSpy(m_server, &WaylandTestServer::configReceived);

    m_server->suspendChanges(true);

    new SetConfigOperation(config, this);

    /* Apply next config */

    auto output2 = config2->outputs()[2]; // is this id stable enough?
    QCOMPARE(output2->scale(), 2.0);
    output2->setScale(3);

    new SetConfigOperation(config2, this);

    QVERIFY(serverReceivedSpy.wait());
    QCOMPARE(serverReceivedSpy.count(), 1);
    m_server->suspendChanges(false);

    QVERIFY(configSpy.wait());
    // check if the server changed
    QCOMPARE(serverSpy.count(), 1);
    QCOMPARE(configSpy.count(), 1);
    QCOMPARE(output->scale(), 2.0);
    QCOMPARE(output2->scale(), 3.0);

    QVERIFY(configSpy.wait());
    // check if the server changed
    QCOMPARE(serverSpy.count(), 2);
    QCOMPARE(configSpy.count(), 2);
    QCOMPARE(output2->scale(), 3.0);
}


QTEST_GUILESS_MAIN(TestKWaylandConfig)

#include "testkwaylandconfig.moc"
