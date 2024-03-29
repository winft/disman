/*************************************************************************************
 *  Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
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
#include "getconfigoperation.h"
#include "mode.h"
#include "output.h"
#include "screen.h"
#include "setconfigoperation.h"

using namespace Disman;

class testScreenConfig : public QObject
{
    Q_OBJECT

private:
    Disman::ConfigPtr getConfig();

private Q_SLOTS:
    void initTestCase();
    void singleOutput();
    void singleOutputWithoutPreferred();
    void multiOutput();
    void configCanBeApplied();
    void supported_features();
    void testInvalidMode();
    void cleanupTestCase();
    void testOutputPositionNormalization();
};

ConfigPtr testScreenConfig::getConfig()
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

void testScreenConfig::initTestCase()
{
    qputenv("DISMAN_LOGGING", "false");
    qputenv("DISMAN_BACKEND", "fake");
}

void testScreenConfig::cleanupTestCase()
{
    BackendManager::instance()->shutdown_backend();
}

void testScreenConfig::singleOutput()
{
    // json file for the fake backend
    qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "singleoutput.json");

    const ConfigPtr config = getConfig();
    QVERIFY(config);
    const ScreenPtr screen = config->screen();
    QVERIFY(screen);

    QCOMPARE(screen->min_size(), QSize(320, 200));
    QCOMPARE(screen->max_size(), QSize(8192, 8192));
    QCOMPARE(screen->current_size(), QSize(1280, 800));

    QCOMPARE(config->outputs().size(), 1);

    const OutputPtr output = config->outputs().at(1);
    QVERIFY(output);

    QVERIFY(config->primary_output());
    QCOMPARE(config->primary_output()->id(), output->id());

    QCOMPARE(output->name(), "LVDS1");
    QCOMPARE(output->type(), Output::Panel);
    QCOMPARE(output->modes().size(), 3);
    QCOMPARE(output->position(), QPoint(0, 0));
    QCOMPARE(output->geometry(), QRect(0, 0, 1280, 800));
    QCOMPARE(output->auto_mode()->id(), "3");
    QCOMPARE(output->preferred_mode()->id(), "3");
    QCOMPARE(output->rotation(), Output::None);
    QCOMPARE(output->scale(), 1.0);
    QCOMPARE(output->enabled(), true);
    // QCOMPARE(output->isEmbedded(), true);

    const ModePtr mode = output->auto_mode();
    QCOMPARE(mode->size(), QSize(1280, 800));
    QCOMPARE(mode->refresh(), 59900);
}

void testScreenConfig::singleOutputWithoutPreferred()
{
    qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "singleOutputWithoutPreferred.json");

    const ConfigPtr config = getConfig();
    QVERIFY(config);
    const OutputPtr output = config->outputs().at(1);
    QVERIFY(output);

    QVERIFY(output->preferred_modes().empty());
    QCOMPARE(output->preferred_mode()->id(), "3");
}

void testScreenConfig::multiOutput()
{
    qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "multipleoutput.json");

    const ConfigPtr config = getConfig();
    QVERIFY(config);
    const ScreenPtr screen = config->screen();
    QVERIFY(screen);

    QCOMPARE(screen->min_size(), QSize(320, 200));
    QCOMPARE(screen->max_size(), QSize(8192, 8192));
    QCOMPARE(screen->current_size(), QSize(3200, 1880));

    QCOMPARE(config->outputs().size(), 2);

    const OutputPtr output = config->outputs().at(2);
    QVERIFY(output);

    QVERIFY(config->primary_output());
    QVERIFY(config->primary_output()->id() != output->id());

    QCOMPARE(output->name(), "HDMI1");
    QCOMPARE(output->type(), Output::HDMI);
    QCOMPARE(output->modes().size(), 4);
    QCOMPARE(output->position(), QPoint(1280, 0));
    QCOMPARE(output->geometry(), QRect(1280, 0, 1920 / 1.4, 1080 / 1.4));
    QCOMPARE(output->auto_mode()->id(), "4");
    QCOMPARE(output->preferred_mode()->id(), "4");
    QCOMPARE(output->rotation(), Output::None);
    QCOMPARE(output->scale(), 1.4);
    QCOMPARE(output->enabled(), true);

    const ModePtr mode = output->auto_mode();
    QVERIFY(mode);
    QCOMPARE(mode->size(), QSize(1920, 1080));
    QCOMPARE(mode->refresh(), 60000);
}

void testScreenConfig::configCanBeApplied()
{
    qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "singleoutputBroken.json");
    const ConfigPtr brokenConfig = getConfig();

    qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "singleoutput.json");
    const ConfigPtr currentConfig = getConfig();
    QVERIFY(currentConfig);
    const OutputPtr primaryBroken = brokenConfig->outputs()[2];
    QVERIFY(primaryBroken);
    const OutputPtr currentPrimary = currentConfig->outputs()[1];
    QVERIFY(currentPrimary);

    QVERIFY(!Config::can_be_applied(brokenConfig));
    primaryBroken->set_id(currentPrimary->id());
    QVERIFY(!Config::can_be_applied(brokenConfig));
    QVERIFY(!Config::can_be_applied(brokenConfig));
    primaryBroken->set_mode(primaryBroken->mode("42"));
    QVERIFY(!Config::can_be_applied(brokenConfig));
    primaryBroken->set_mode(currentPrimary->auto_mode());
    QVERIFY(!Config::can_be_applied(brokenConfig));

    primaryBroken->mode("3")->set_size(QSize(1280, 800));
    QVERIFY(Config::can_be_applied(brokenConfig));

    qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "tooManyOutputs.json");
    const ConfigPtr brokenConfig2 = getConfig();
    QVERIFY(brokenConfig2);

    int enabledOutputsCount = 0;
    for (auto const& [key, output] : brokenConfig2->outputs()) {
        if (output->enabled()) {
            ++enabledOutputsCount;
        }
    }
    QVERIFY(brokenConfig2->screen()->max_outputs_count() < enabledOutputsCount);
    QVERIFY(!Config::can_be_applied(brokenConfig2));

    const ConfigPtr nulllConfig;
    QVERIFY(!Config::can_be_applied(nulllConfig));
}

void testScreenConfig::supported_features()
{
    ConfigPtr config = getConfig();

    QVERIFY(config->supported_features().testFlag(Disman::Config::Feature::None));
    QVERIFY(!config->supported_features().testFlag(Disman::Config::Feature::Writable));
    QVERIFY(!config->supported_features().testFlag(Disman::Config::Feature::PrimaryDisplay));
    QVERIFY(!config->supported_features().testFlag(Disman::Config::Feature::PerOutputScaling));

    config->set_supported_features(Disman::Config::Feature::Writable
                                   | Disman::Config::Feature::PrimaryDisplay);
    QVERIFY(config->supported_features().testFlag(Disman::Config::Feature::Writable));
    QVERIFY(config->supported_features().testFlag(Disman::Config::Feature::PrimaryDisplay));

    config->set_supported_features(Disman::Config::Feature::None);
    QVERIFY(config->supported_features().testFlag(Disman::Config::Feature::None));

    config->set_supported_features(Disman::Config::Feature::PerOutputScaling
                                   | Disman::Config::Feature::Writable);
    QVERIFY(!config->supported_features().testFlag(Disman::Config::Feature::None));
    QVERIFY(config->supported_features().testFlag(Disman::Config::Feature::Writable));
    QVERIFY(config->supported_features().testFlag(Disman::Config::Feature::PerOutputScaling));

    config->set_supported_features(Disman::Config::Feature::PerOutputScaling
                                   | Disman::Config::Feature::Writable
                                   | Disman::Config::Feature::PrimaryDisplay);
    QVERIFY(!config->supported_features().testFlag(Disman::Config::Feature::None));
    QVERIFY(config->supported_features().testFlag(Disman::Config::Feature::Writable));
    QVERIFY(config->supported_features().testFlag(Disman::Config::Feature::PrimaryDisplay));
    QVERIFY(config->supported_features().testFlag(Disman::Config::Feature::PerOutputScaling));
}

void testScreenConfig::testInvalidMode()
{
    auto output = new Disman::Output();
    auto currentMode = output->auto_mode();
    QVERIFY(!currentMode);
    delete output;
}

void testScreenConfig::testOutputPositionNormalization()
{
    qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "multipleoutput.json");

    const ConfigPtr config = getConfig();
    QVERIFY(config);
    auto left = config->outputs().begin()->second;
    auto right = config->outputs().rbegin()->second;
    QVERIFY(left);
    QVERIFY(right);
    left->set_position(QPoint(-5000, 700));
    right->set_position(QPoint(-3720, 666));
    QCOMPARE(left->position(), QPoint(-5000, 700));
    QCOMPARE(right->position(), QPoint(-3720, 666));

    // start a set operation to fix up the positions
    {
        auto setop = new SetConfigOperation(config);
        setop->exec();
    }
    QCOMPARE(left->position(), QPoint(0, 34));
    QCOMPARE(right->position(), QPoint(1280, 0));

    // make sure it doesn't touch a valid config
    {
        auto setop = new SetConfigOperation(config);
        setop->exec();
    }
    QCOMPARE(left->position(), QPoint(0, 34));
    QCOMPARE(right->position(), QPoint(1280, 0));

    // positions of single outputs should be at 0, 0
    left->set_enabled(false);
    {
        auto setop = new SetConfigOperation(config);
        setop->exec();
    }
    QCOMPARE(right->position(), QPoint());
}

QTEST_MAIN(testScreenConfig)

#include "testscreenconfig.moc"
