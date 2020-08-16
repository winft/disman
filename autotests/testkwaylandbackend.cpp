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

#include "../src/backendmanager_p.h"
#include "../src/config.h"
#include "../src/configmonitor.h"
#include "../src/edid.h"
#include "../src/getconfigoperation.h"
#include "../src/mode.h"
#include "../src/output.h"
#include "../src/setconfigoperation.h"

// KWayland
#include <KWayland/Server/display.h>
#include <KWayland/Server/outputdevice_interface.h>

#include "waylandtestserver.h"

Q_LOGGING_CATEGORY(DISMAN_WAYLAND, "disman.wayland.kwayland")

using namespace Disman;

class testWaylandBackend : public QObject
{
    Q_OBJECT

public:
    explicit testWaylandBackend(QObject* parent = nullptr);

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
    void testEdid();

private:
    ConfigPtr m_config;
    WaylandTestServer* m_server;
    KWayland::Server::OutputDeviceInterface* m_serverOutputDevice;
};

testWaylandBackend::testWaylandBackend(QObject* parent)
    : QObject(parent)
    , m_config(nullptr)
{
    qputenv("DISMAN_IN_PROCESS", "1");
    qputenv("DISMAN_LOGGING", "false");
    QStandardPaths::setTestModeEnabled(true);

    m_server = new WaylandTestServer(this);
    m_server->setConfig(QLatin1String(TEST_DATA) + QLatin1String("multipleoutput.json"));
}

void testWaylandBackend::init()
{
    qputenv("DISMAN_BACKEND", "wayland");

    // This is how KWayland will pick up the right socket,
    // and thus connect to our internal test server.
    setenv("WAYLAND_DISPLAY", s_socketName.toLocal8Bit().constData(), 1);
    m_server->start();

    GetConfigOperation* op = new GetConfigOperation();
    op->exec();
    m_config = op->config();
    QVERIFY(m_config);
}

void testWaylandBackend::cleanup()
{
    Disman::BackendManager::instance()->shutdownBackend();
    m_server->stop();

    // Make sure we did not accidentally unset test mode prior and delete our user configuration.
    QStandardPaths::setTestModeEnabled(true);
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        % QStringLiteral("/disman/");
    QVERIFY(QDir(path).removeRecursively());
}

void testWaylandBackend::loadConfig()
{
    GetConfigOperation* op = new GetConfigOperation();
    op->exec();
    m_config = op->config();
    QVERIFY(m_config->isValid());
    qCDebug(DISMAN_WAYLAND) << "Outputs:" << m_config->outputs();
}

void testWaylandBackend::verifyConfig()
{
    QVERIFY(m_config != nullptr);
}

void testWaylandBackend::verifyScreen()
{
    ScreenPtr screen = m_config->screen();

    QVERIFY(screen->minSize().width() <= screen->maxSize().width());
    QVERIFY(screen->minSize().height() <= screen->maxSize().height());

    QVERIFY(screen->minSize().width() <= screen->currentSize().width());
    QVERIFY(screen->minSize().height() <= screen->currentSize().height());

    QVERIFY(screen->maxSize().width() >= screen->currentSize().width());
    QVERIFY(screen->maxSize().height() >= screen->currentSize().height());
    QVERIFY(m_config->screen()->maxActiveOutputsCount() > 0);
}

void testWaylandBackend::verifyOutputs()
{
    bool primaryFound = false;
    Q_FOREACH (const Disman::OutputPtr op, m_config->outputs()) {
        if (op->isPrimary()) {
            primaryFound = true;
        }
    }
    // qCDebug(DISMAN_WAYLAND) << "Primary found? " << primaryFound << m_config->outputs();
    QVERIFY(primaryFound);
    QVERIFY(m_config->outputs().count());
    QCOMPARE(m_server->outputCount(), m_config->outputs().count());

    Disman::OutputPtr primary = m_config->primaryOutput();
    QVERIFY(primary->isEnabled());

    QList<int> ids;
    Q_FOREACH (const auto& output, m_config->outputs()) {
        QVERIFY(!output->name().isEmpty());
        QVERIFY(output->id() > -1);
        QVERIFY(output->geometry() != QRectF(1, 1, 1, 1));
        QVERIFY(output->geometry() != QRectF());
        QVERIFY(output->sizeMm() != QSize());
        QVERIFY(output->edid() != nullptr);
        QVERIFY(output->preferredModes().size() == 1);
        QCOMPARE(output->rotation(), Output::None);
        QVERIFY(!ids.contains(output->id()));
        ids << output->id();
    }
}

void testWaylandBackend::verifyModes()
{
    Disman::OutputPtr primary = m_config->primaryOutput();
    QVERIFY(primary);
    QVERIFY(primary->modes().count() > 0);

    Q_FOREACH (const auto& output, m_config->outputs()) {
        Q_FOREACH (auto mode, output->modes()) {
            QVERIFY(!mode->name().isEmpty());
            QVERIFY(mode->refreshRate() > 0);
            QVERIFY(mode->size().isValid());
        }
    }
}

void testWaylandBackend::verifyIds()
{
    QList<quint32> ids;
    Q_FOREACH (const auto& output, m_config->outputs()) {
        QVERIFY(ids.contains(output->id()) == false);
        QVERIFY(output->id() > 0);
        ids << output->id();
    }
}

void testWaylandBackend::simpleWrite()
{
    Disman::BackendManager::instance()->shutdownBackend();
    GetConfigOperation* op = new GetConfigOperation();
    op->exec();
    m_config = op->config();
    auto output = m_config->output(18);
    QVERIFY(output);

    auto n_mode = QStringLiteral("1");
    output->set_mode(output->mode(n_mode));
    QCOMPARE(output->commanded_mode()->size(), QSize(800, 600));

    auto setop = new SetConfigOperation(m_config);
    QVERIFY(setop->exec());
}

void testWaylandBackend::addAndRemoveOutput()
{
    Disman::BackendManager::instance()->shutdownBackend();
    GetConfigOperation* op = new GetConfigOperation();
    op->exec();
    auto config = op->config();
    QCOMPARE(config->outputs().count(), 2);
    Disman::ConfigMonitor* monitor = Disman::ConfigMonitor::instance();
    monitor->addConfig(config);
    QSignalSpy configSpy(monitor, &Disman::ConfigMonitor::configurationChanged);

    // Now add an outputdevice on the server side
    m_serverOutputDevice = m_server->display()->createOutputDevice(this);
    m_serverOutputDevice->setUuid("1337");

    OutputDeviceInterface::Mode m0;
    m0.id = 0;
    m0.size = QSize(800, 600);
    m0.flags = OutputDeviceInterface::ModeFlags(OutputDeviceInterface::ModeFlag::Preferred);
    m_serverOutputDevice->addMode(m0);

    OutputDeviceInterface::Mode m1;
    m1.id = 1;
    m1.size = QSize(1024, 768);
    m_serverOutputDevice->addMode(m1);

    OutputDeviceInterface::Mode m2;
    m2.id = 2;
    m2.size = QSize(1280, 1024);
    m2.refreshRate = 90000;
    m_serverOutputDevice->addMode(m2);

    m_serverOutputDevice->setCurrentMode(1);

    m_serverOutputDevice->create();

    QVERIFY(configSpy.wait());
    // QTRY_VERIFY(configSpy.count());

    GetConfigOperation* op2 = new GetConfigOperation();
    op2->exec();
    auto newconfig = op2->config();
    QCOMPARE(newconfig->outputs().count(), 3);

    // Now remove the output again.
    delete m_serverOutputDevice;
    QVERIFY(configSpy.wait());
    GetConfigOperation* op3 = new GetConfigOperation();
    op3->exec();
    newconfig = op3->config();
    QCOMPARE(newconfig->outputs().count(), 2);
}

void testWaylandBackend::testEdid()
{
    m_server->showOutputs();

    QByteArray data = QByteArray::fromBase64(
        "AP///////"
        "wAQrBbwTExLQQ4WAQOANCB46h7Frk80sSYOUFSlSwCBgKlA0QBxTwEBAQEBAQEBKDyAoHCwI0AwIDYABkQhAAAaAAA"
        "A/wBGNTI1TTI0NUFLTEwKAAAA/ABERUxMIFUyNDEwCiAgAAAA/"
        "QA4TB5REQAKICAgICAgAToCAynxUJAFBAMCBxYBHxITFCAVEQYjCQcHZwMMABAAOC2DAQAA4wUDAQI6gBhxOC1AWCx"
        "FAAZEIQAAHgEdgBhxHBYgWCwlAAZEIQAAngEdAHJR0B4gbihVAAZEIQAAHowK0Iog4C0QED6WAAZEIQAAGAAAAAAAA"
        "AAAAAAAAAAAPg==");

    QScopedPointer<Edid> edid(new Edid(data));
    QVERIFY(edid->isValid());

    GetConfigOperation* op = new GetConfigOperation();
    op->exec();
    auto config = op->config();
    QVERIFY(config->outputs().count() > 0);

    auto o = config->outputs().last();
    qCDebug(DISMAN_WAYLAND) << "Edid: " << o->edid()->isValid();
    QVERIFY(o->edid()->isValid());
    QCOMPARE(o->edid()->deviceId(), edid->deviceId());
    QCOMPARE(o->edid()->name(), edid->name());
    QCOMPARE(o->edid()->vendor(), edid->vendor());
    QCOMPARE(o->edid()->eisaId(), edid->eisaId());
    QCOMPARE(o->edid()->serial(), edid->serial());
    QCOMPARE(o->edid()->hash(), edid->hash());
    QCOMPARE(o->edid()->width(), edid->width());
    QCOMPARE(o->edid()->height(), edid->height());
    QCOMPARE(o->edid()->gamma(), edid->gamma());
    QCOMPARE(o->edid()->red(), edid->red());
    QCOMPARE(o->edid()->green(), edid->green());
    QCOMPARE(o->edid()->blue(), edid->blue());
    QCOMPARE(o->edid()->white(), edid->white());
}

void testWaylandBackend::verifyFeatures()
{
    GetConfigOperation* op = new GetConfigOperation();
    op->exec();
    auto config = op->config();
    QVERIFY(!config->supportedFeatures().testFlag(Config::Feature::None));
    QVERIFY(config->supportedFeatures().testFlag(Config::Feature::Writable));
    QVERIFY(!config->supportedFeatures().testFlag(Config::Feature::PrimaryDisplay));
}

QTEST_GUILESS_MAIN(testWaylandBackend)

#include "testkwaylandbackend.moc"
