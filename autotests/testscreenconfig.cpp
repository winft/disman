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

#include "../src/backendmanager_p.h"
#include "../src/config.h"
#include "../src/getconfigoperation.h"
#include "../src/mode.h"
#include "../src/output.h"
#include "../src/screen.h"
#include "../src/setconfigoperation.h"

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
    void supportedFeatures();
    void testInvalidMode();
    void cleanupTestCase();
    void testOutputPositionNormalization();
};

ConfigPtr testScreenConfig::getConfig()
{
    qputenv("DISMAN_BACKEND_INPROCESS", "1");
    auto* op = new GetConfigOperation();
    if (!op->exec()) {
        qWarning("ConfigOperation error: %s", qPrintable(op->errorString()));
        BackendManager::instance()->shutdownBackend();
        return ConfigPtr();
    }

    BackendManager::instance()->shutdownBackend();

    return op->config();
}

void testScreenConfig::initTestCase()
{
    qputenv("DISMAN_LOGGING", "false");
    qputenv("DISMAN_BACKEND", "Fake");
}

void testScreenConfig::cleanupTestCase()
{
    BackendManager::instance()->shutdownBackend();
}

void testScreenConfig::singleOutput()
{
    // json file for the fake backend
    qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "singleoutput.json");

    const ConfigPtr config = getConfig();
    QVERIFY(!config.isNull());
    const ScreenPtr screen = config->screen();
    QVERIFY(!screen.isNull());

    QCOMPARE(screen->minSize(), QSize(320, 200));
    QCOMPARE(screen->maxSize(), QSize(8192, 8192));
    QCOMPARE(screen->currentSize(), QSize(1280, 800));

    QCOMPARE(config->outputs().count(), 1);

    const OutputPtr output = config->outputs().take(1);
    QVERIFY(!output.isNull());

    QCOMPARE(output->name(), QLatin1String("LVDS1"));
    QCOMPARE(output->type(), Output::Panel);
    QCOMPARE(output->modes().count(), 3);
    QCOMPARE(output->position(), QPoint(0, 0));
    QCOMPARE(output->geometry(), QRect(0, 0, 1280, 800));
    QCOMPARE(output->currentModeId(), QLatin1String("3"));
    QCOMPARE(output->preferredModeId(), QLatin1String("3"));
    QCOMPARE(output->rotation(), Output::None);
    QCOMPARE(output->scale(), 1.0);
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->isPrimary(), true);
    // QCOMPARE(output->isEmbedded(), true);

    const ModePtr mode = output->currentMode();
    QCOMPARE(mode->size(), QSize(1280, 800));
    QCOMPARE(mode->refreshRate(), (float)59.9);
}

void testScreenConfig::singleOutputWithoutPreferred()
{
    qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "singleOutputWithoutPreferred.json");

    const ConfigPtr config = getConfig();
    QVERIFY(!config.isNull());
    const OutputPtr output = config->outputs().take(1);
    QVERIFY(!output.isNull());

    QVERIFY(output->preferredModes().isEmpty());
    QCOMPARE(output->preferredModeId(), QLatin1String("3"));
}

void testScreenConfig::multiOutput()
{
    qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "multipleoutput.json");

    const ConfigPtr config = getConfig();
    QVERIFY(!config.isNull());
    const ScreenPtr screen = config->screen();
    QVERIFY(!screen.isNull());

    QCOMPARE(screen->minSize(), QSize(320, 200));
    QCOMPARE(screen->maxSize(), QSize(8192, 8192));
    QCOMPARE(screen->currentSize(), QSize(3200, 1880));

    QCOMPARE(config->outputs().count(), 2);

    const OutputPtr output = config->outputs().take(2);
    QVERIFY(!output.isNull());

    QCOMPARE(output->name(), QStringLiteral("HDMI1"));
    QCOMPARE(output->type(), Output::HDMI);
    QCOMPARE(output->modes().count(), 4);
    QCOMPARE(output->position(), QPoint(1280, 0));
    QCOMPARE(output->geometry(), QRect(1280, 0, 1920 / 1.4, 1080 / 1.4));
    QCOMPARE(output->currentModeId(), QLatin1String("4"));
    QCOMPARE(output->preferredModeId(), QLatin1String("4"));
    QCOMPARE(output->rotation(), Output::None);
    QCOMPARE(output->scale(), 1.4);
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->isPrimary(), false);

    const ModePtr mode = output->currentMode();
    QVERIFY(!mode.isNull());
    QCOMPARE(mode->size(), QSize(1920, 1080));
    QCOMPARE(mode->refreshRate(), (float)60.0);
}

void testScreenConfig::configCanBeApplied()
{
    qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "singleoutputBroken.json");
    const ConfigPtr brokenConfig = getConfig();

    qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "singleoutput.json");
    const ConfigPtr currentConfig = getConfig();
    QVERIFY(!currentConfig.isNull());
    const OutputPtr primaryBroken = brokenConfig->outputs()[2];
    QVERIFY(!primaryBroken.isNull());
    const OutputPtr currentPrimary = currentConfig->outputs()[1];
    QVERIFY(!currentPrimary.isNull());

    QVERIFY(!Config::canBeApplied(brokenConfig));
    primaryBroken->setId(currentPrimary->id());
    QVERIFY(!Config::canBeApplied(brokenConfig));
    QVERIFY(!Config::canBeApplied(brokenConfig));
    primaryBroken->setCurrentModeId(QStringLiteral("42"));
    QVERIFY(!Config::canBeApplied(brokenConfig));
    primaryBroken->setCurrentModeId(currentPrimary->currentModeId());
    QVERIFY(!Config::canBeApplied(brokenConfig));
    qDebug() << "brokenConfig.modes" << primaryBroken->mode(QStringLiteral("3"));
    primaryBroken->mode(QStringLiteral("3"))->setSize(QSize(1280, 800));
    qDebug() << "brokenConfig.modes" << primaryBroken->mode(QStringLiteral("3"));
    QVERIFY(Config::canBeApplied(brokenConfig));

    qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "tooManyOutputs.json");
    const ConfigPtr brokenConfig2 = getConfig();
    QVERIFY(!brokenConfig2.isNull());

    int enabledOutputsCount = 0;
    Q_FOREACH (const OutputPtr& output, brokenConfig2->outputs()) {
        if (output->isEnabled()) {
            ++enabledOutputsCount;
        }
    }
    QVERIFY(brokenConfig2->screen()->maxActiveOutputsCount() < enabledOutputsCount);
    QVERIFY(!Config::canBeApplied(brokenConfig2));

    const ConfigPtr nulllConfig;
    QVERIFY(!Config::canBeApplied(nulllConfig));
}

void testScreenConfig::supportedFeatures()
{
    ConfigPtr config = getConfig();

    QVERIFY(config->supportedFeatures().testFlag(Disman::Config::Feature::None));
    QVERIFY(!config->supportedFeatures().testFlag(Disman::Config::Feature::Writable));
    QVERIFY(!config->supportedFeatures().testFlag(Disman::Config::Feature::PrimaryDisplay));
    QVERIFY(!config->supportedFeatures().testFlag(Disman::Config::Feature::PerOutputScaling));

    config->setSupportedFeatures(Disman::Config::Feature::Writable
                                 | Disman::Config::Feature::PrimaryDisplay);
    QVERIFY(config->supportedFeatures().testFlag(Disman::Config::Feature::Writable));
    QVERIFY(config->supportedFeatures().testFlag(Disman::Config::Feature::PrimaryDisplay));

    config->setSupportedFeatures(Disman::Config::Feature::None);
    QVERIFY(config->supportedFeatures().testFlag(Disman::Config::Feature::None));

    config->setSupportedFeatures(Disman::Config::Feature::PerOutputScaling
                                 | Disman::Config::Feature::Writable);
    QVERIFY(!config->supportedFeatures().testFlag(Disman::Config::Feature::None));
    QVERIFY(config->supportedFeatures().testFlag(Disman::Config::Feature::Writable));
    QVERIFY(config->supportedFeatures().testFlag(Disman::Config::Feature::PerOutputScaling));

    config->setSupportedFeatures(Disman::Config::Feature::PerOutputScaling
                                 | Disman::Config::Feature::Writable
                                 | Disman::Config::Feature::PrimaryDisplay);
    QVERIFY(!config->supportedFeatures().testFlag(Disman::Config::Feature::None));
    QVERIFY(config->supportedFeatures().testFlag(Disman::Config::Feature::Writable));
    QVERIFY(config->supportedFeatures().testFlag(Disman::Config::Feature::PrimaryDisplay));
    QVERIFY(config->supportedFeatures().testFlag(Disman::Config::Feature::PerOutputScaling));
}

void testScreenConfig::testInvalidMode()
{
    ModeList modes;
    ModePtr invalidMode = modes.value(QStringLiteral("99"));
    QVERIFY(invalidMode.isNull());

    auto output = new Disman::Output();
    auto currentMode = output->currentMode();
    QVERIFY(currentMode.isNull());
    QVERIFY(!currentMode);
    delete output;
}

void testScreenConfig::testOutputPositionNormalization()
{
    qputenv("DISMAN_BACKEND_ARGS", "TEST_DATA=" TEST_DATA "multipleoutput.json");

    const ConfigPtr config = getConfig();
    QVERIFY(!config.isNull());
    auto left = config->outputs().first();
    auto right = config->outputs().last();
    QVERIFY(!left.isNull());
    QVERIFY(!right.isNull());
    left->setPosition(QPoint(-5000, 700));
    right->setPosition(QPoint(-3720, 666));
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
    left->setEnabled(false);
    {
        auto setop = new SetConfigOperation(config);
        setop->exec();
    }
    QCOMPARE(right->position(), QPoint());
}

QTEST_MAIN(testScreenConfig)

#include "testscreenconfig.moc"
