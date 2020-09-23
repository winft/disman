/*
    SPDX-FileCopyrightText: 2020 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only
*/
#include <QCoreApplication>
#include <QObject>
#include <QtTest>
#include <memory>

#include "config.h"
#include "getconfigoperation.h"
#include "output.h"

using namespace Disman;

class TestConfig : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void test_load();
    void test_compare_simple_data();
    void test_compare_outputs();

private:
    ConfigPtr load_config(std::string file_name);
};

void TestConfig::initTestCase()
{
    qputenv("DISMAN_LOGGING", "false");
    qputenv("DISMAN_IN_PROCESS", "1");
    qputenv("DISMAN_BACKEND", "fake");
}

ConfigPtr TestConfig::load_config(std::string file_name)
{
    auto test_data = std::string("TEST_DATA=") + std::string(TEST_DATA) + file_name;
    qputenv("DISMAN_BACKEND_ARGS", test_data.c_str());

    auto op = new GetConfigOperation();
    if (!op->exec()) {
        qDebug() << "GetConfigOperation::exec failed.";
        return nullptr;
    }

    auto config = op->config();
    if (!config) {
        qDebug() << "GetConfigOperation returned null config.";
        return nullptr;
    }
    if (!config->valid()) {
        qDebug() << "GetConfigOperation returned invalid config.";
        return nullptr;
    }

    return config;
}

void TestConfig::test_load()
{
    auto config = load_config("singleoutput.json");
    QVERIFY(config);
    QVERIFY(config->valid());
}

void TestConfig::test_compare_simple_data()
{
    auto config = load_config("singleoutput.json");
    QVERIFY(config);

    QVERIFY(!config->compare(nullptr));
    QVERIFY(config->compare(config));

    auto config2 = config->clone();
    QVERIFY(config2);

    QVERIFY(config->compare(config2));
    QVERIFY(config2->compare(config));

    config2->set_valid(false);
    QVERIFY(!config->compare(config2));
    QVERIFY(!config2->compare(config));

    config2->set_valid(true);
    QVERIFY(config->compare(config2));

    QCOMPARE(config2->cause(), Config::Cause::file);
    config2->set_cause(Config::Cause::generated);
    QVERIFY(!config->compare(config2));

    config2->set_cause(Config::Cause::file);
    QVERIFY(config->compare(config2));
}

void TestConfig::test_compare_outputs()
{
    auto config = load_config("multipleoutput.json");
    QVERIFY(config);

    auto config2 = config->clone();
    QVERIFY(config2);

    QVERIFY(config->compare(config2));

    QVERIFY(config->outputs().at(1));
    QVERIFY(config->outputs().at(2));

    auto output3 = config2->outputs().at(2)->clone();

    QVERIFY(output3);
    output3->set_id(3);

    config2->add_output(output3);

    QVERIFY(!config->compare(config2));
    QVERIFY(!config2->compare(config));

    config2->remove_output(3);
    QVERIFY(config->compare(config2));

    config2->remove_output(2);
    QVERIFY(!config->compare(config2));
    QVERIFY(!config2->compare(config));
}

QTEST_GUILESS_MAIN(TestConfig)

#include "config.moc"
