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
    BackendManager::instance()->shutdownBackend();
}

Disman::ConfigPtr TestGenerator::load_config(const QByteArray& file_name)
{
    qputenv("DISMAN_IN_PROCESS", "1");
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
    config->setSupportedFeatures(Config::Feature::PrimaryDisplay);

    // First make the received config worse.
    config->setPrimaryOutput(nullptr);

    auto output = config->outputs().value(1);
    output->setEnabled(false);
    output->set_mode(output->modes().value(QStringLiteral("2")));

    QVERIFY(!config->primaryOutput());
    QCOMPARE(output->isEnabled(), false);
    QCOMPARE(output->auto_mode()->id(), QStringLiteral("2"));

    // Now optimize the config.
    Generator generator(config);
    QVERIFY(generator.optimize());

    auto generated_config = generator.config();
    output = generated_config->outputs().value(1);

    QVERIFY(generated_config->primaryOutput());
    QCOMPARE(generated_config->primaryOutput()->id(), output->id());

    QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->position(), QPoint(0, 0));
}

void TestGenerator::multi_output_embedded()
{
    auto config = load_config("multipleoutput.json");
    QVERIFY(config);
    QCOMPARE(config->outputs().size(), 2);
    config->setSupportedFeatures(Config::Feature::PerOutputScaling
                                 | Config::Feature::PrimaryDisplay);

    // First make the received config worse.
    config->setPrimaryOutput(nullptr);
    QVERIFY(!config->primaryOutput());

    for (auto output : config->outputs()) {
        output->setEnabled(false);
        output->set_mode(output->modes().value(QStringLiteral("3")));

        QCOMPARE(output->isEnabled(), false);
        QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    }

    Generator generator(config);
    QVERIFY(generator.optimize());

    auto generated_config = generator.config();
    auto output = generated_config->outputs().value(1);

    QVERIFY(generated_config->primaryOutput());
    QCOMPARE(generated_config->primaryOutput()->id(), output->id());

    QCOMPARE(generator.embedded(), output);
    QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->position(), QPoint(0, 0));

    output = generated_config->outputs().value(2);

    QCOMPARE(generator.biggest(), output);
    QCOMPARE(output->auto_mode()->id(), QStringLiteral("4"));
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->position(), QPointF(1280, 0));
}

void TestGenerator::replicate_embedded()
{
    auto config = load_config("multipleoutput.json");
    QVERIFY(config);
    QCOMPARE(config->outputs().size(), 2);
    config->setSupportedFeatures(Config::Feature::PerOutputScaling
                                 | Config::Feature::OutputReplication
                                 | Config::Feature::PrimaryDisplay);

    // First make the received config worse.
    config->setPrimaryOutput(nullptr);
    QVERIFY(!config->primaryOutput());

    for (auto output : config->outputs()) {
        output->setEnabled(false);
        output->set_mode(output->modes().value(QStringLiteral("3")));

        QCOMPARE(output->isEnabled(), false);
        QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    }

    Generator generator(config);
    QVERIFY(generator.replicate());

    auto generated_config = generator.config();
    auto output = generated_config->outputs().value(1);

    QVERIFY(generated_config->primaryOutput());
    QCOMPARE(generated_config->primaryOutput()->id(), output->id());

    QCOMPARE(generator.embedded(), output);
    QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->position(), QPoint(0, 0));
    QCOMPARE(output->replicationSource(), 0);

    output = generated_config->outputs().value(2);
    QCOMPARE(generator.biggest(), output);
    QCOMPARE(output->auto_mode()->id(), QStringLiteral("4"));
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->replicationSource(), 1);
}

void TestGenerator::multi_output_pc()
{
    auto config = load_config("multipleoutput.json");
    QVERIFY(config);
    QCOMPARE(config->outputs().size(), 2);
    config->setSupportedFeatures(Config::Feature::PerOutputScaling
                                 | Config::Feature::PrimaryDisplay);

    // First make the received config worse.
    config->setPrimaryOutput(nullptr);
    QVERIFY(!config->primaryOutput());

    for (auto output : config->outputs()) {
        output->setEnabled(false);
        output->set_mode(output->modes().value(QStringLiteral("3")));
        output->setType(Output::Type::HDMI);

        QCOMPARE(output->isEnabled(), false);
        QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    }

    Generator generator(config);
    QVERIFY(generator.optimize());

    auto generated_config = generator.config();
    auto output = generated_config->outputs().value(1);

    QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->position(), QPoint(1920, 0));

    output = generated_config->outputs().value(2);
    QVERIFY(generated_config->primaryOutput());
    QCOMPARE(generated_config->primaryOutput()->id(), output->id());

    QCOMPARE(generator.biggest(), output);
    QCOMPARE(output->auto_mode()->id(), QStringLiteral("4"));
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->position(), QPointF(0, 0));
}

void TestGenerator::replicate_pc()
{
    auto config = load_config("multipleoutput.json");
    QVERIFY(config);
    QCOMPARE(config->outputs().size(), 2);
    config->setSupportedFeatures(Config::Feature::PerOutputScaling
                                 | Config::Feature::OutputReplication
                                 | Config::Feature::PrimaryDisplay);

    // First make the received config worse.
    config->setPrimaryOutput(nullptr);
    QVERIFY(!config->primaryOutput());

    for (auto output : config->outputs()) {
        output->setEnabled(false);
        output->set_mode(output->modes().value(QStringLiteral("3")));
        output->setType(Output::Type::HDMI);

        QCOMPARE(output->isEnabled(), false);
        QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    }

    Generator generator(config);
    QVERIFY(generator.replicate());

    auto generated_config = generator.config();
    auto output = generated_config->outputs().value(1);

    QCOMPARE(output->auto_mode()->id(), QStringLiteral("3"));
    QCOMPARE(output->isEnabled(), true);
    QCOMPARE(output->position(), QPoint(0, 0));
    QCOMPARE(output->replicationSource(), 2);

    output = generated_config->outputs().value(2);
    QVERIFY(generated_config->primaryOutput());
    QCOMPARE(generated_config->primaryOutput()->id(), output->id());

    QCOMPARE(generator.biggest(), output);
    QCOMPARE(output->auto_mode()->id(), QStringLiteral("4"));
    QCOMPARE(output->isEnabled(), true);

    // Position is unchanged from initial config.
    QCOMPARE(output->position(), QPoint(1280, 0));
    QCOMPARE(output->replicationSource(), 0);
}

QTEST_GUILESS_MAIN(TestGenerator)

#include "generator.moc"
