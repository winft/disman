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
    BackendManager::instance()->shutdown_backend();
}

Disman::ConfigPtr TestGenerator::load_config(const QByteArray& file_name)
{
    qputenv("DISMAN_IN_PROCESS", "1");
    auto path = "TEST_DATA=" TEST_DATA;
    qputenv("DISMAN_BACKEND_ARGS", path + file_name);

    auto op = new GetConfigOperation();
    if (!op->exec()) {
        qWarning("ConfigOperation error: %s", qPrintable(op->error_string()));
        BackendManager::instance()->shutdown_backend();
        return ConfigPtr();
    }

    BackendManager::instance()->shutdown_backend();
    return op->config();
}

void TestGenerator::single_output()
{
    auto config = load_config("singleoutput.json");
    QVERIFY(config);
    config->set_supported_features(Config::Feature::PrimaryDisplay);

    // First make the received config worse.
    config->set_primary_output(nullptr);

    auto output = config->outputs().at(1);
    output->set_enabled(false);
    output->set_mode(output->modes().at("2"));

    QVERIFY(!config->primary_output());
    QCOMPARE(output->enabled(), false);
    QCOMPARE(output->auto_mode()->id(), "2");

    // Now optimize the config.
    Generator generator(config);
    QVERIFY(generator.optimize());

    auto generated_config = generator.config();
    output = generated_config->outputs().at(1);

    QVERIFY(generated_config->primary_output());
    QCOMPARE(generated_config->primary_output()->id(), output->id());

    QCOMPARE(output->auto_mode()->id(), "3");
    QCOMPARE(output->enabled(), true);
    QCOMPARE(output->position(), QPoint(0, 0));
}

void TestGenerator::multi_output_embedded()
{
    auto config = load_config("multipleoutput.json");
    QVERIFY(config);
    QCOMPARE(config->outputs().size(), 2);
    config->set_supported_features(Config::Feature::PerOutputScaling
                                   | Config::Feature::PrimaryDisplay);

    // First make the received config worse.
    config->set_primary_output(nullptr);
    QVERIFY(!config->primary_output());

    for (auto const& [key, output] : config->outputs()) {
        output->set_enabled(false);
        output->set_mode(output->modes().at("3"));

        QCOMPARE(output->enabled(), false);
        QCOMPARE(output->auto_mode()->id(), "3");
    }

    Generator generator(config);
    QVERIFY(generator.optimize());

    auto generated_config = generator.config();
    auto output = generated_config->outputs().at(1);

    QVERIFY(generated_config->primary_output());
    QCOMPARE(generated_config->primary_output()->id(), output->id());

    QCOMPARE(generator.embedded(), output);
    QCOMPARE(output->auto_mode()->id(), "3");
    QCOMPARE(output->enabled(), true);
    QCOMPARE(output->position(), QPoint(0, 0));

    output = generated_config->outputs().at(2);

    QCOMPARE(generator.biggest(), output);
    QCOMPARE(output->auto_mode()->id(), "4");
    QCOMPARE(output->enabled(), true);
    QCOMPARE(output->position(), QPointF(1280, 0));
}

void TestGenerator::replicate_embedded()
{
    auto config = load_config("multipleoutput.json");
    QVERIFY(config);
    QCOMPARE(config->outputs().size(), 2);
    config->set_supported_features(Config::Feature::PerOutputScaling
                                   | Config::Feature::OutputReplication
                                   | Config::Feature::PrimaryDisplay);

    // First make the received config worse.
    config->set_primary_output(nullptr);
    QVERIFY(!config->primary_output());

    for (auto const& [key, output] : config->outputs()) {
        output->set_enabled(false);
        output->set_mode(output->modes().at("3"));

        QCOMPARE(output->enabled(), false);
        QCOMPARE(output->auto_mode()->id(), "3");
    }

    Generator generator(config);
    QVERIFY(generator.replicate());

    auto generated_config = generator.config();
    auto output = generated_config->outputs().at(1);

    QVERIFY(generated_config->primary_output());
    QCOMPARE(generated_config->primary_output()->id(), output->id());

    QCOMPARE(generator.embedded(), output);
    QCOMPARE(output->auto_mode()->id(), "3");
    QCOMPARE(output->enabled(), true);
    QCOMPARE(output->position(), QPoint(0, 0));
    QCOMPARE(output->replication_source(), 0);

    output = generated_config->outputs().at(2);
    QCOMPARE(generator.biggest(), output);
    QCOMPARE(output->auto_mode()->id(), "4");
    QCOMPARE(output->enabled(), true);
    QCOMPARE(output->replication_source(), 1);
}

void TestGenerator::multi_output_pc()
{
    auto config = load_config("multipleoutput.json");
    QVERIFY(config);
    QCOMPARE(config->outputs().size(), 2);
    config->set_supported_features(Config::Feature::PerOutputScaling
                                   | Config::Feature::PrimaryDisplay);

    // First make the received config worse.
    config->set_primary_output(nullptr);
    QVERIFY(!config->primary_output());

    for (auto const& [key, output] : config->outputs()) {
        output->set_enabled(false);
        output->set_mode(output->modes().at("3"));
        output->setType(Output::Type::HDMI);

        QCOMPARE(output->enabled(), false);
        QCOMPARE(output->auto_mode()->id(), "3");
    }

    Generator generator(config);
    QVERIFY(generator.optimize());

    auto generated_config = generator.config();
    auto output = generated_config->outputs().at(1);

    QCOMPARE(output->auto_mode()->id(), "3");
    QCOMPARE(output->enabled(), true);
    QCOMPARE(output->position(), QPoint(1920, 0));

    output = generated_config->outputs().at(2);
    QVERIFY(generated_config->primary_output());
    QCOMPARE(generated_config->primary_output()->id(), output->id());

    QCOMPARE(generator.biggest(), output);
    QCOMPARE(output->auto_mode()->id(), "4");
    QCOMPARE(output->enabled(), true);
    QCOMPARE(output->position(), QPointF(0, 0));
}

void TestGenerator::replicate_pc()
{
    auto config = load_config("multipleoutput.json");
    QVERIFY(config);
    QCOMPARE(config->outputs().size(), 2);
    config->set_supported_features(Config::Feature::PerOutputScaling
                                   | Config::Feature::OutputReplication
                                   | Config::Feature::PrimaryDisplay);

    // First make the received config worse.
    config->set_primary_output(nullptr);
    QVERIFY(!config->primary_output());

    for (auto const& [key, output] : config->outputs()) {
        output->set_enabled(false);
        output->set_mode(output->modes().at("3"));
        output->setType(Output::Type::HDMI);

        QCOMPARE(output->enabled(), false);
        QCOMPARE(output->auto_mode()->id(), "3");
    }

    Generator generator(config);
    QVERIFY(generator.replicate());

    auto generated_config = generator.config();
    auto output = generated_config->outputs().at(1);

    QCOMPARE(output->auto_mode()->id(), "3");
    QCOMPARE(output->enabled(), true);
    QCOMPARE(output->position(), QPoint(0, 0));
    QCOMPARE(output->replication_source(), 2);

    output = generated_config->outputs().at(2);
    QVERIFY(generated_config->primary_output());
    QCOMPARE(generated_config->primary_output()->id(), output->id());

    QCOMPARE(generator.biggest(), output);
    QCOMPARE(output->auto_mode()->id(), "4");
    QCOMPARE(output->enabled(), true);

    // Position is unchanged from initial config.
    QCOMPARE(output->position(), QPoint(1280, 0));
    QCOMPARE(output->replication_source(), 0);
}

QTEST_GUILESS_MAIN(TestGenerator)

#include "generator.moc"
