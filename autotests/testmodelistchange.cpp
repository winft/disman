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

class TestModeMapChange : public QObject
{
    Q_OBJECT

private:
    Disman::ConfigPtr getConfig();
    Disman::ModeMap createModeMap();
    bool compareModeMap(Disman::ModeMap before, Disman::ModeMap& after);

    QSize s0 = QSize(1920, 1080);
    QSize s1 = QSize(1600, 1200);
    QSize s2 = QSize(1280, 1024);
    QSize s3 = QSize(800, 600);
    QSize snew = QSize(777, 888);
    std::string idnew = "666";

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void modeListChange();
};

ConfigPtr TestModeMapChange::getConfig()
{
    qputenv("DISMAN_IN_PROCESS", "1");
    auto* op = new GetConfigOperation();
    if (!op->exec()) {
        qWarning("ConfigOperation error: %s", qPrintable(op->error_string()));
        BackendManager::instance()->shutdown_backend();
        return ConfigPtr();
    }

    BackendManager::instance()->shutdown_backend();

    return op->config();
}

Disman::ModeMap TestModeMapChange::createModeMap()
{

    Disman::ModeMap newmodes;
    {
        auto _id = std::to_string(11);
        Disman::ModePtr dismanMode(new Disman::Mode);
        dismanMode->set_id(_id);
        dismanMode->set_name(_id);
        dismanMode->set_size(s0);
        dismanMode->set_refresh(60);
        newmodes.insert({_id, dismanMode});
    }
    {
        auto _id = std::to_string(22);
        Disman::ModePtr dismanMode(new Disman::Mode);
        dismanMode->set_id(_id);
        dismanMode->set_name(_id);
        dismanMode->set_size(s1);
        dismanMode->set_refresh(60);
        newmodes.insert({_id, dismanMode});
    }
    {
        auto _id = std::to_string(33);
        Disman::ModePtr dismanMode(new Disman::Mode);
        dismanMode->set_id(_id);
        dismanMode->set_name(_id);
        dismanMode->set_size(s2);
        dismanMode->set_refresh(60);
        newmodes.insert({_id, dismanMode});
    }
    return newmodes;
}

void TestModeMapChange::initTestCase()
{
    qputenv("DISMAN_LOGGING", "false");
    qputenv("DISMAN_BACKEND", "Fake");
}

void TestModeMapChange::cleanupTestCase()
{
    BackendManager::instance()->shutdown_backend();
}

void TestModeMapChange::modeListChange()
{
    // json file for the fake backend
    qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "singleoutput.json");

    const ConfigPtr config = getConfig();
    QVERIFY(config);

    auto output = config->outputs().begin()->second;
    QVERIFY(output);
    auto modelist = output->modes();

    auto mode = modelist.begin()->second;
    mode->set_id("44");
    mode->set_size(QSize(880, 440));
    output->set_modes(modelist);

    QCOMPARE(output->modes().begin()->second->id(), "44");
    QCOMPARE(output->modes().begin()->second->size(), QSize(880, 440));
    QVERIFY(!modelist.empty());

    ConfigMonitor::instance()->add_config(config);

    auto before = createModeMap();
    output->set_modes(before);
    output->set_modes(before);
    output->set_modes(before);
    QCOMPARE(output->modes().begin()->second->size(), s0);
    QCOMPARE(output->modes().begin()->second->id(), "11");

    auto after = createModeMap();
    auto firstmode = after.begin()->second;
    QVERIFY(firstmode);
    QCOMPARE(firstmode->size(), s0);
    QCOMPARE(firstmode->id(), "11");
    firstmode->set_size(snew);
    firstmode->set_id(idnew);
    output->set_modes(after);

    auto _id = std::to_string(11);
    Disman::ModePtr dismanMode(new Disman::Mode);
    dismanMode->set_id(_id);
    dismanMode->set_name(_id);
    dismanMode->set_size(s0);
    dismanMode->set_refresh(60);
    before.insert({_id, dismanMode});
    output->set_modes(before);
    QCOMPARE(output->modes().size(), 3);

    auto _id2 = std::to_string(999);
    Disman::ModePtr dismanMode2(new Disman::Mode);
    dismanMode2->set_id(_id2);
    dismanMode2->set_name(_id2);
    dismanMode2->set_size(s0);
    dismanMode2->set_refresh(60);
    before.insert({_id2, dismanMode2});
    output->set_modes(before);
    QCOMPARE(output->modes().size(), 4);
    QCOMPARE(output->modes()[_id2]->id(), _id2);
}

QTEST_MAIN(TestModeMapChange)

#include "testmodelistchange.moc"
