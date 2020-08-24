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
#include <QObject>
#include <QtTest>

#include "backendmanager_p.h"
#include "config.h"
#include "configmonitor.h"
#include "getconfigoperation.h"
#include "mode.h"
#include "output.h"
#include "setconfigoperation.h"

using namespace Disman;

class TestModeListChange : public QObject
{
    Q_OBJECT

private:
    Disman::ConfigPtr getConfig();
    Disman::ModeList createModeList();
    bool compareModeList(Disman::ModeList before, Disman::ModeList& after);

    QSize s0 = QSize(1920, 1080);
    QSize s1 = QSize(1600, 1200);
    QSize s2 = QSize(1280, 1024);
    QSize s3 = QSize(800, 600);
    QSize snew = QSize(777, 888);
    QString idnew = QStringLiteral("666");

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void modeListChange();
};

ConfigPtr TestModeListChange::getConfig()
{
    qputenv("DISMAN_IN_PROCESS", "1");
    auto* op = new GetConfigOperation();
    if (!op->exec()) {
        qWarning("ConfigOperation error: %s", qPrintable(op->errorString()));
        BackendManager::instance()->shutdownBackend();
        return ConfigPtr();
    }

    BackendManager::instance()->shutdownBackend();

    return op->config();
}

Disman::ModeList TestModeListChange::createModeList()
{

    Disman::ModeList newmodes;
    {
        QString _id = QString::number(11);
        Disman::ModePtr dismanMode(new Disman::Mode);
        dismanMode->setId(_id);
        dismanMode->setName(_id);
        dismanMode->setSize(s0);
        dismanMode->setRefreshRate(60);
        newmodes.insert(_id, dismanMode);
    }
    {
        QString _id = QString::number(22);
        Disman::ModePtr dismanMode(new Disman::Mode);
        dismanMode->setId(_id);
        dismanMode->setName(_id);
        dismanMode->setSize(s1);
        dismanMode->setRefreshRate(60);
        newmodes.insert(_id, dismanMode);
    }
    {
        QString _id = QString::number(33);
        Disman::ModePtr dismanMode(new Disman::Mode);
        dismanMode->setId(_id);
        dismanMode->setName(_id);
        dismanMode->setSize(s2);
        dismanMode->setRefreshRate(60);
        newmodes.insert(_id, dismanMode);
    }
    return newmodes;
}

void TestModeListChange::initTestCase()
{
    qputenv("DISMAN_LOGGING", "false");
    qputenv("DISMAN_BACKEND", "Fake");
}

void TestModeListChange::cleanupTestCase()
{
    BackendManager::instance()->shutdownBackend();
}

void TestModeListChange::modeListChange()
{
    // json file for the fake backend
    qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "singleoutput.json");

    const ConfigPtr config = getConfig();
    QVERIFY(!config.isNull());

    auto output = config->outputs().first();
    QVERIFY(!output.isNull());
    auto modelist = output->modes();

    auto mode = modelist.first();
    mode->setId(QStringLiteral("44"));
    mode->setSize(QSize(880, 440));
    output->setModes(modelist);

    QCOMPARE(output->modes().first()->id(), QStringLiteral("44"));
    QCOMPARE(output->modes().first()->size(), QSize(880, 440));
    QVERIFY(!modelist.isEmpty());

    ConfigMonitor::instance()->addConfig(config);

    auto before = createModeList();
    output->setModes(before);
    output->setModes(before);
    output->setModes(before);
    QCOMPARE(output->modes().first()->size(), s0);
    QCOMPARE(output->modes().first()->id(), QStringLiteral("11"));

    auto after = createModeList();
    auto firstmode = after.first();
    QVERIFY(!firstmode.isNull());
    QCOMPARE(firstmode->size(), s0);
    QCOMPARE(firstmode->id(), QStringLiteral("11"));
    firstmode->setSize(snew);
    firstmode->setId(idnew);
    output->setModes(after);

    QString _id = QString::number(11);
    Disman::ModePtr dismanMode(new Disman::Mode);
    dismanMode->setId(_id);
    dismanMode->setName(_id);
    dismanMode->setSize(s0);
    dismanMode->setRefreshRate(60);
    before.insert(_id, dismanMode);
    output->setModes(before);
    QCOMPARE(output->modes().size(), 3);

    QString _id2 = QString::number(999);
    Disman::ModePtr dismanMode2(new Disman::Mode);
    dismanMode2->setId(_id2);
    dismanMode2->setName(_id2);
    dismanMode2->setSize(s0);
    dismanMode2->setRefreshRate(60);
    before.insert(_id2, dismanMode2);
    output->setModes(before);
    QCOMPARE(output->modes().size(), 4);
    QCOMPARE(output->modes()[_id2]->id(), _id2);
}

QTEST_MAIN(TestModeListChange)

#include "testmodelistchange.moc"
