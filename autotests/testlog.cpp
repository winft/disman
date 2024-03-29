/*************************************************************************************
 *  Copyright 2016 by Sebastian Kügler <sebas@kde.org>                               *
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
#include <QLoggingCategory>
#include <QObject>
#include <QtTest>

#include "log.h"

Q_DECLARE_LOGGING_CATEGORY(DISMAN_TESTLOG)

Q_LOGGING_CATEGORY(DISMAN_TESTLOG, "disman.testlog")

using namespace Disman;

auto DISMAN_LOGGING = "DISMAN_LOGGING";

class TestLog : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void initTestCase();
    void cleanupTestCase();
    void testContext();
    void testEnabled();
    void testLog();

private:
    QString m_defaultLogFile;
};

void TestLog::init()
{
    QStandardPaths::setTestModeEnabled(true);
    m_defaultLogFile = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QLatin1String("/disman/disman.log");
}

void TestLog::initTestCase()
{

    qputenv(DISMAN_LOGGING, QByteArray("true"));
}

void TestLog::cleanupTestCase()
{
    qunsetenv(DISMAN_LOGGING);
}

void TestLog::testContext()
{
    auto log = Log::instance();
    QString ctx = QStringLiteral("context text");
    QVERIFY(log != nullptr);
    log->set_context(ctx);
    QCOMPARE(log->context(), ctx);

    delete log;
}

void TestLog::testEnabled()
{
    qputenv(DISMAN_LOGGING, QByteArray("faLSe"));

    auto log = Log::instance();
    QCOMPARE(log->enabled(), false);
    QCOMPARE(log->file(), QString());

    delete log;
    qunsetenv(DISMAN_LOGGING);

    log = Log::instance();
    QCOMPARE(log->enabled(), false);
    QCOMPARE(log->file(), QString());

    delete log;
    qputenv(DISMAN_LOGGING, QByteArray("truE"));

    log = Log::instance();
    QCOMPARE(log->enabled(), true);
    QCOMPARE(log->file(), m_defaultLogFile);

    delete log;
}

void TestLog::testLog()
{
    auto log = Log::instance();
    Q_UNUSED(log);

    QFile lf(m_defaultLogFile);
    lf.remove();
    QVERIFY(!lf.exists());

    QString logmsg = QStringLiteral("This is a log message. ♥");
    Log::log(logmsg);

    QVERIFY(lf.exists());
    QVERIFY(lf.remove());

    qCDebug(DISMAN_TESTLOG) << "qCDebug message from testlog";
    QVERIFY(lf.exists());
    QVERIFY(lf.remove());

    delete Log::instance();

    // Make sure on log file gets written when disabled
    qputenv(DISMAN_LOGGING, "false");

    qCDebug(DISMAN_TESTLOG) << logmsg;
    QCOMPARE(Log::instance()->enabled(), false);
    QVERIFY(!lf.exists());

    Log::log(logmsg);
    QVERIFY(!lf.exists());

    // Make sure we don't crash on cleanup
    delete Log::instance();
    delete Log::instance();
}

QTEST_MAIN(TestLog)

#include "testlog.moc"
