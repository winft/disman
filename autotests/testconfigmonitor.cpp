/*
 * Copyright (C) 2014  Daniel Vratil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#include <QObject>
#include <QSignalSpy>
#include <QtTest>

#include "backendmanager_p.h"
#include "config.h"
#include "configmonitor.h"
#include "configoperation.h"
#include "getconfigoperation.h"
#include "output.h"
#include "setconfigoperation.h"
#include <QSignalSpy>

#include "fakebackendinterface.h"

class TestConfigMonitor : public QObject
{
    Q_OBJECT
public:
    TestConfigMonitor()
    {
    }

    Disman::ConfigPtr getConfig()
    {
        auto op = new Disman::GetConfigOperation();
        if (!op->exec()) {
            qWarning("Failed to retrieve backend: %s", qPrintable(op->error_string()));
            return Disman::ConfigPtr();
        }

        return op->config();
    }

private Q_SLOTS:
    void initTestCase()
    {
        qputenv("DISMAN_LOGGING", "false");
        qputenv("DISMAN_BACKEND", "fake");
        // This particular test is only useful for out of process operation, so enforce that
        qputenv("DISMAN_IN_PROCESS", "0");
        Disman::BackendManager::instance()->shutdown_backend();
    }

    void cleanupTestCase()
    {
        Disman::BackendManager::instance()->shutdown_backend();
    }

    void testChangeNotifyInProcess()
    {
        qputenv("DISMAN_IN_PROCESS", "1");
        Disman::BackendManager::instance()->shutdown_backend();
        Disman::BackendManager::instance()->set_method(Disman::BackendManager::InProcess);
        // json file for the fake backend
        qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "singleoutput.json");

        // Prepare monitor
        Disman::ConfigMonitor* monitor = Disman::ConfigMonitor::instance();
        QSignalSpy spy(monitor, SIGNAL(configuration_changed()));

        // Get config and monitor it for changes
        Disman::ConfigPtr config = getConfig();
        monitor->add_config(config);

        auto output = config->outputs().first();

        output->set_enabled(false);
        auto setop = new Disman::SetConfigOperation(config);
        QVERIFY(!setop->has_error());
        setop->exec();
        QTRY_VERIFY(!spy.isEmpty());

        QCOMPARE(spy.size(), 1);
        QCOMPARE(config->output(1)->enabled(), false);

        output->set_enabled(false);
        auto setop2 = new Disman::SetConfigOperation(config);
        QVERIFY(!setop2->has_error());
        setop2->exec();
        QTRY_VERIFY(!spy.isEmpty());
        QCOMPARE(spy.size(), 2);
    }
};

QTEST_MAIN(TestConfigMonitor)

#include "testconfigmonitor.moc"
