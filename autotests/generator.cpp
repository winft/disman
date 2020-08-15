/*************************************************************************
Copyright © 2020   Roman Gilg <subdiff@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
**************************************************************************/
#include <QtTest>

#include "backendmanager_p.h"
#include "generator.h"
#include "getconfigoperation.h"
#include "output.h"

using namespace Disman;

class TestGenerator : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void single_output();
    void multi_output_embedded();
    void replicate_embedded();
    void multi_output_pc();
    void replicate_pc();
    //    void workstationWithoutScreens();
    //    void workstationWithNoConnectedScreens();
    //    void workstationTwoExternalSameSize();
    //    void workstationFallbackMode();
    //    void workstationTwoExternalDiferentSize();

private:
    Disman::ConfigPtr load_config(QByteArray const& file_name);
};

void TestGenerator::initTestCase()
{
    qputenv("DISMAN_LOGGING", "false");
    qputenv("DISMAN_BACKEND", "fake");
}

void TestGenerator::cleanupTestCase()
{
    BackendManager::instance()->shutdownBackend();
}

Disman::ConfigPtr TestGenerator::load_config(const QByteArray& file_name)
{
    qputenv("DISMAN_BACKEND_INPROCESS", "1");
    auto path = "TEST_DATA=" TEST_DATA;
    qputenv("DISMAN_BACKEND_ARGS", path + file_name);

    auto op = new GetConfigOperation();
    if (!op->exec()) {
        qWarning("ConfigOperation error: %s", qPrintable(op->errorString()));
        BackendManager::instance()->shutdownBackend();
        return ConfigPtr();
    }

    BackendManager::instance()->shutdownBackend();
    return op->config();
}

void TestGenerator::single_output()
{
    auto config = load_config("singleoutput.json");
    QVERIFY(config);

    // First make the received config worse.
    auto output = config->outputs().value(1);
    output->setEnabled(false);
    output->setPrimary(false);
    output->set_mode(output->modes().value(QStringLiteral("2")));

    QCOMPARE(output->isEnabled(), false);
    QCOMPARE(output->isPrimary(), false);
    QCOMPARE(output->auto_mode()->id(), QStringLiteral("2"));

    // Now optimize the config.
    Generator generator(config);
    QVERIFY(generator.optimize());
    output = generator.config()->outputs().value(1);

    QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->isPrimary(), true);
    QCOMPARE(output->position(), QPoint(0, 0));
}

void TestGenerator::multi_output_embedded()
{
    auto config = load_config("multipleoutput.json");
    QVERIFY(config);
    QCOMPARE(config->outputs().size(), 2);
    config->setSupportedFeatures(Config::Feature::PerOutputScaling);

    // First make the received config worse.
    for (auto output : config->outputs()) {
        output->setEnabled(false);
        output->setPrimary(false);
        output->set_mode(output->modes().value(QStringLiteral("3")));

        QCOMPARE(output->isEnabled(), false);
        QCOMPARE(output->isPrimary(), false);
        QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    }

    Generator generator(config);
    QVERIFY(generator.optimize());

    auto output = generator.config()->outputs().value(1);
    QCOMPARE(generator.embedded(), output);
    QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->isPrimary(), true);
    QCOMPARE(output->position(), QPoint(0, 0));

    output = generator.config()->outputs().value(2);
    QCOMPARE(generator.biggest(), output);
    QCOMPARE(output->auto_mode()->id(), QStringLiteral("4"));
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->isPrimary(), false);
    QCOMPARE(output->position(), QPointF(1280, 0));
}

void TestGenerator::replicate_embedded()
{
    auto config = load_config("multipleoutput.json");
    QVERIFY(config);
    QCOMPARE(config->outputs().size(), 2);
    config->setSupportedFeatures(Config::Feature::PerOutputScaling
                                 | Config::Feature::OutputReplication);

    // First make the received config worse.
    for (auto output : config->outputs()) {
        output->setEnabled(false);
        output->setPrimary(false);
        output->set_mode(output->modes().value(QStringLiteral("3")));

        QCOMPARE(output->isEnabled(), false);
        QCOMPARE(output->isPrimary(), false);
        QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    }

    Generator generator(config);
    QVERIFY(generator.replicate());

    auto output = generator.config()->outputs().value(1);
    QCOMPARE(generator.embedded(), output);
    QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->isPrimary(), true);
    QCOMPARE(output->position(), QPoint(0, 0));
    QCOMPARE(output->replicationSource(), 0);

    output = generator.config()->outputs().value(2);
    QCOMPARE(generator.biggest(), output);
    QCOMPARE(output->auto_mode()->id(), QStringLiteral("4"));
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->isPrimary(), false);
    QCOMPARE(output->replicationSource(), 1);
}

void TestGenerator::multi_output_pc()
{
    auto config = load_config("multipleoutput.json");
    QVERIFY(config);
    QCOMPARE(config->outputs().size(), 2);
    config->setSupportedFeatures(Config::Feature::PerOutputScaling);

    // First make the received config worse.
    for (auto output : config->outputs()) {
        output->setEnabled(false);
        output->setPrimary(false);
        output->set_mode(output->modes().value(QStringLiteral("3")));
        output->setType(Output::Type::HDMI);

        QCOMPARE(output->isEnabled(), false);
        QCOMPARE(output->isPrimary(), false);
        QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    }

    Generator generator(config);
    QVERIFY(generator.optimize());

    auto output = generator.config()->outputs().value(1);
    QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->isPrimary(), false);
    QCOMPARE(output->position(), QPoint(1920, 0));

    output = generator.config()->outputs().value(2);
    QCOMPARE(generator.biggest(), output);
    QCOMPARE(output->auto_mode()->id(), QStringLiteral("4"));
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->isPrimary(), true);
    QCOMPARE(output->position(), QPointF(0, 0));
}

void TestGenerator::replicate_pc()
{
    auto config = load_config("multipleoutput.json");
    QVERIFY(config);
    QCOMPARE(config->outputs().size(), 2);
    config->setSupportedFeatures(Config::Feature::PerOutputScaling
                                 | Config::Feature::OutputReplication);

    // First make the received config worse.
    for (auto output : config->outputs()) {
        output->setEnabled(false);
        output->setPrimary(false);
        output->set_mode(output->modes().value(QStringLiteral("3")));
        output->setType(Output::Type::HDMI);

        QCOMPARE(output->isEnabled(), false);
        QCOMPARE(output->isPrimary(), false);
        QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    }

    Generator generator(config);
    QVERIFY(generator.replicate());

    auto output = generator.config()->outputs().value(1);
    QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->isPrimary(), false);
    QCOMPARE(output->position(), QPoint(0, 0));
    QCOMPARE(output->replicationSource(), 2);

    output = generator.config()->outputs().value(2);
    QCOMPARE(generator.biggest(), output);
    QCOMPARE(output->auto_mode()->id(), QStringLiteral("4"));
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->isPrimary(), true);

    // Position is unchanged from initial config.
    QCOMPARE(output->position(), QPoint(1280, 0));
    QCOMPARE(output->replicationSource(), 0);
}

QTEST_GUILESS_MAIN(TestGenerator)

#include "generator.moc"
