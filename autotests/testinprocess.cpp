/*************************************************************************************
 *  Copyright 2015 by Sebastian Kügler <sebas@kde.org>                           *
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
#include <QDBusConnectionInterface>
#include <QObject>
#include <QSignalSpy>
#include <QtTest>

#include "backendmanager_p.h"
#include "config.h"
#include "configmonitor.h"
#include "getconfigoperation.h"
#include "mode.h"
#include "output.h"
#include "setconfigoperation.h"

Q_LOGGING_CATEGORY(DISMAN, "disman")

using namespace Disman;

class TestInProcess : public QObject
{
    Q_OBJECT

public:
    explicit TestInProcess(QObject* parent = nullptr);

private Q_SLOTS:
    void initTestCase();

    void init();
    void cleanup();

    void loadConfig();

    void testCreateJob();
    void testModeSwitching();
    void testBackendCaching();

    void testConfigApply();
    void testConfigMonitor();

private:
    ConfigPtr m_config;
    bool m_backendServiceInstalled = false;
};

TestInProcess::TestInProcess(QObject* parent)
    : QObject(parent)
    , m_config(nullptr)
{
}

void TestInProcess::initTestCase()
{
    m_backendServiceInstalled = true;

    const QString dismanServiceName = QStringLiteral("Disman");
    QDBusConnectionInterface* bus = QDBusConnection::sessionBus().interface();
    if (!bus->isServiceRegistered(dismanServiceName)) {
        auto reply = bus->startService(dismanServiceName);
        if (!reply.isValid()) {
            qDebug() << "D-Bus service Disman could not be started, skipping out-of-process tests";
            m_backendServiceInstalled = false;
        }
    }
}

void TestInProcess::init()
{
    qputenv("DISMAN_LOGGING", "false");
    // Make sure we do everything in-process
    qputenv("DISMAN_IN_PROCESS", "1");
    // Use Fake backend with one of the json configs
    qputenv("DISMAN_BACKEND", "fake");
    qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "multipleoutput.json");

    Disman::BackendManager::instance()->shutdown_backend();
}

void TestInProcess::cleanup()
{
    Disman::BackendManager::instance()->shutdown_backend();
}

void TestInProcess::loadConfig()
{
    qputenv("DISMAN_IN_PROCESS", "1");
    BackendManager::instance()->set_method(BackendManager::InProcess);

    auto* op = new GetConfigOperation();
    QVERIFY(op->exec());
    m_config = op->config();
    QVERIFY(m_config);
    QVERIFY(m_config->valid());
}

void TestInProcess::testModeSwitching()
{
    Disman::BackendManager::instance()->shutdown_backend();
    BackendManager::instance()->set_method(BackendManager::InProcess);
    // Load QScreen backend in-process
    qDebug() << "TT qscreen in-process";
    qputenv("DISMAN_BACKEND", "qscreen");
    auto op = new GetConfigOperation();
    QVERIFY(op->exec());
    auto oc = op->config();
    QVERIFY(oc != nullptr);
    QVERIFY(oc->valid());

    qDebug() << "TT fake in-process";
    // Load the Fake backend in-process
    qputenv("DISMAN_BACKEND", "fake");
    auto ip = new GetConfigOperation();
    QVERIFY(ip->exec());
    auto ic = ip->config();
    QVERIFY(ic != nullptr);
    QVERIFY(ic->valid());
    QVERIFY(ic->outputs().size());

    Disman::ConfigPtr xc(nullptr);
    if (m_backendServiceInstalled) {
        qDebug() << "TT xrandr out-of-process";
        // Load the xrandr backend out-of-process
        qputenv("DISMAN_BACKEND", "qscreen");
        qputenv("DISMAN_IN_PROCESS", "0");
        BackendManager::instance()->set_method(BackendManager::OutOfProcess);
        auto xp = new GetConfigOperation();
        QCOMPARE(BackendManager::instance()->method(), BackendManager::OutOfProcess);
        QVERIFY(xp->exec());
        xc = xp->config();
        QVERIFY(xc != nullptr);
        QVERIFY(xc->valid());
        QVERIFY(xc->outputs().size());
    }

    qDebug() << "TT fake in-process";

    qputenv("DISMAN_IN_PROCESS", "1");
    BackendManager::instance()->set_method(BackendManager::InProcess);
    // Load the Fake backend in-process
    qputenv("DISMAN_BACKEND", "fake");
    auto fp = new GetConfigOperation();
    QCOMPARE(BackendManager::instance()->method(), BackendManager::InProcess);
    QVERIFY(fp->exec());
    auto fc = fp->config();
    QVERIFY(fc != nullptr);
    QVERIFY(fc->valid());
    QVERIFY(fc->outputs().size());

    QVERIFY(oc->valid());
    QVERIFY(ic->valid());
    if (xc) {
        QVERIFY(xc->valid());
    }
    QVERIFY(fc->valid());
}

void TestInProcess::testBackendCaching()
{
    Disman::BackendManager::instance()->shutdown_backend();
    qputenv("DISMAN_BACKEND", "fake");
    QElapsedTimer t;
    BackendManager::instance()->set_method(BackendManager::InProcess);
    QCOMPARE(BackendManager::instance()->method(), BackendManager::InProcess);
    int t_cold;
    int t_warm;

    {
        t.start();
        auto cp = new GetConfigOperation();
        cp->exec();
        auto cc = cp->config();
        t_cold = t.nsecsElapsed();
        QVERIFY(cc != nullptr);
        QVERIFY(cc->valid());
        QVERIFY(cc->outputs().size());
    }
    {
        // Disman::BackendManager::instance()->shutdown_backend();
        QCOMPARE(BackendManager::instance()->method(), BackendManager::InProcess);
        t.start();
        auto cp = new GetConfigOperation();
        cp->exec();
        auto cc = cp->config();
        t_warm = t.nsecsElapsed();
        QVERIFY(cc != nullptr);
        QVERIFY(cc->valid());
        QVERIFY(cc->outputs().size());
    }
    {
        auto cp = new GetConfigOperation();
        QCOMPARE(BackendManager::instance()->method(), BackendManager::InProcess);
        cp->exec();
        auto cc = cp->config();
        QVERIFY(cc != nullptr);
        QVERIFY(cc->valid());
        QVERIFY(cc->outputs().size());
    }
    // Check if all our configs are still valid after the backend is gone
    Disman::BackendManager::instance()->shutdown_backend();

    if (m_backendServiceInstalled) {
        // qputenv("DISMAN_BACKEND", "qscreen");
        qputenv("DISMAN_IN_PROCESS", "0");
        BackendManager::instance()->set_method(BackendManager::OutOfProcess);
        QCOMPARE(BackendManager::instance()->method(), BackendManager::OutOfProcess);
        int t_x_cold;

        {
            t.start();
            auto xp = new GetConfigOperation();
            xp->exec();
            t_x_cold = t.nsecsElapsed();
            auto xc = xp->config();
            QVERIFY(xc != nullptr);
        }
        t.start();
        auto xp = new GetConfigOperation();
        xp->exec();
        int t_x_warm = t.nsecsElapsed();
        auto xc = xp->config();
        QVERIFY(xc != nullptr);

        // Make sure in-process is faster
        QVERIFY(t_cold > t_warm);
        QVERIFY(t_x_cold > t_x_warm);
        QVERIFY(t_x_cold > t_cold);
        return;
        qDebug() << "ip  speedup for cached access:" << (qreal)((qreal)t_cold / (qreal)t_warm);
        qDebug() << "oop speedup for cached access:" << (qreal)((qreal)t_x_cold / (qreal)t_x_warm);
        qDebug() << "out-of vs. in-process speedup:" << (qreal)((qreal)t_x_warm / (qreal)t_warm);
        qDebug() << "cold oop:   " << ((qreal)t_x_cold / 1000000);
        qDebug() << "cached oop: " << ((qreal)t_x_warm / 1000000);
        qDebug() << "cold in process:   " << ((qreal)t_cold / 1000000);
        qDebug() << "cached in process: " << ((qreal)t_warm / 1000000);
    }
}

void TestInProcess::testCreateJob()
{
    Disman::BackendManager::instance()->shutdown_backend();
    {
        BackendManager::instance()->set_method(BackendManager::InProcess);
        auto op = new GetConfigOperation();
        auto _op = qobject_cast<GetConfigOperation*>(op);
        QVERIFY(_op != nullptr);
        QCOMPARE(BackendManager::instance()->method(), BackendManager::InProcess);
        QVERIFY(op->exec());
        auto cc = op->config();
        QVERIFY(cc != nullptr);
        QVERIFY(cc->valid());
    }
    if (m_backendServiceInstalled) {
        BackendManager::instance()->set_method(BackendManager::OutOfProcess);
        auto op = new GetConfigOperation();
        auto _op = qobject_cast<GetConfigOperation*>(op);
        QVERIFY(_op != nullptr);
        QCOMPARE(BackendManager::instance()->method(), BackendManager::OutOfProcess);
        QVERIFY(op->exec());
        auto cc = op->config();
        QVERIFY(cc != nullptr);
        QVERIFY(cc->valid());
    }
    Disman::BackendManager::instance()->shutdown_backend();
    BackendManager::instance()->set_method(BackendManager::InProcess);
}

void TestInProcess::testConfigApply()
{
    qputenv("DISMAN_BACKEND", "fake");
    Disman::BackendManager::instance()->shutdown_backend();
    BackendManager::instance()->set_method(BackendManager::InProcess);
    auto op = new GetConfigOperation();
    op->exec();
    auto config = op->config();
    //     qDebug() << "op:" << config->outputs().size();
    auto output = config->outputs().begin()->second;
    //     qDebug() << "res:" << output->geometry();
    //     qDebug() << "modes:" << output->modes();
    auto m0 = output->modes().begin()->second;
    // qDebug() << "m0:" << m0->id() << m0;
    output->set_mode(m0);
    QVERIFY(Config::can_be_applied(config));

    // expected to fail, SetConfigOperation is out-of-process only
    auto setop = new SetConfigOperation(config);
    QVERIFY(!setop->has_error());
    QVERIFY(setop->exec());

    QVERIFY(!setop->has_error());
}

void TestInProcess::testConfigMonitor()
{
    qputenv("DISMAN_BACKEND", "fake");

    Disman::BackendManager::instance()->shutdown_backend();
    BackendManager::instance()->set_method(BackendManager::InProcess);
    auto op = new GetConfigOperation();
    op->exec();
    auto config = op->config();
    //     qDebug() << "op:" << config->outputs().size();
    auto output = config->outputs().begin()->second;
    //     qDebug() << "res:" << output->geometry();
    //     qDebug() << "modes:" << output->modes();
    auto m0 = output->modes().begin()->second;
    // qDebug() << "m0:" << m0->id() << m0;
    output->set_mode(m0);
    QVERIFY(Config::can_be_applied(config));

    QSignalSpy monitorSpy(ConfigMonitor::instance(), &ConfigMonitor::configuration_changed);
    qDebug() << "MOnitorspy connencted.";
    ConfigMonitor::instance()->add_config(config);

    auto setop = new SetConfigOperation(config);
    QVERIFY(!setop->has_error());
    // do not cal setop->exec(), this must not block as the signalspy already blocks
    QVERIFY(monitorSpy.wait(500));
}

QTEST_GUILESS_MAIN(TestInProcess)

#include "testinprocess.moc"
