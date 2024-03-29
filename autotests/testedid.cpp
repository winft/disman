/*************************************************************************************
 *  Copyright 2018 by Frederik Gladhorn <gladhorn@kde.org>                           *
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
#include <QCoreApplication>
#include <QObject>
#include <QtTest>
#include <memory>

#include "edid.h"

using namespace Disman;

Q_DECLARE_METATYPE(std::string)

class TestEdid : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testInvalid();
    void testEdidParser_data();
    void testEdidParser();
};

void TestEdid::initTestCase()
{
    qputenv("DISMAN_LOGGING", "false");
    qputenv("DISMAN_IN_PROCESS", "1");
    qputenv("DISMAN_BACKEND", "fake");
}

void TestEdid::testInvalid()
{
    std::unique_ptr<Edid> e(new Edid("some random data"));
    QCOMPARE(e->isValid(), false);
    QCOMPARE(e->name().size(), 0);

    std::unique_ptr<Edid> e2(new Edid(*e.get()));
    QCOMPARE(e2->isValid(), false);
    QCOMPARE(e2->name().size(), 0);
}

void TestEdid::testEdidParser_data()
{
    // The raw edid data
    QTest::addColumn<QByteArray>("raw_edid");
    QTest::addColumn<std::string>("deviceId");
    QTest::addColumn<std::string>("name");
    QTest::addColumn<std::string>("pnpId");
    // List of potential vendor names, this depends on the availablility
    // of pnp.ids, otherwise it will be a three letter abbreviation.
    QTest::addColumn<QStringList>("vendor");
    QTest::addColumn<std::string>("serial");
    QTest::addColumn<std::string>("eisaId");
    QTest::addColumn<std::string>("hash");
    QTest::addColumn<uint>("width");
    QTest::addColumn<uint>("height");
    QTest::addColumn<qreal>("gamma");

    QTest::addColumn<QQuaternion>("red");
    QTest::addColumn<QQuaternion>("green");
    QTest::addColumn<QQuaternion>("blue");
    QTest::addColumn<QQuaternion>("white");

    QTest::addRow("cor") << QByteArray::fromBase64(
        "AP///////"
        "wAN8iw0AAAAABwVAQOAHRB4CoPVlFdSjCccUFQAAAABAQEBAQEBAQEBAQEBAQEBEhtWWlAAGTAwIDYAJaQQAAAYEht"
        "WWlAAGTAwIDYAJaQQAAAYAAAA/gBBVU8KICAgICAgICAgAAAA/gBCMTMzWFcwMyBWNCAKAIc=")
                         // device-id
                         << std::string("xrandr-Corollary Inc")
                         // name
                         << std::string("")
                         // pnp-id, vendor
                         << std::string("COR")
                         << QStringList({QStringLiteral("COR"), QStringLiteral("Corollary Inc")})
                         // serial, eisa-id
                         << std::string("")
                         << std::string("B133XW03 V4")
                         // hash, width, height
                         << std::string("82266089b3f9da3a8c48de1ec81b09e1") << 29U
                         << 16U
                         // gamma
                         << 2.2
                         // colors: rgbw
                         << QQuaternion(1, QVector3D(0.580078, 0.339844, 0))
                         << QQuaternion(1, QVector3D(0.320313, 0.549805, 0))
                         << QQuaternion(1, QVector3D(0.155273, 0.110352, 0))
                         << QQuaternion(1, QVector3D(0.313477, 0.329102, 0));

    QTest::addRow("dell") << QByteArray::fromBase64(
        "AP///////"
        "wAQrBbwTExLQQ4WAQOANCB46h7Frk80sSYOUFSlSwCBgKlA0QBxTwEBAQEBAQEBKDyAoHCwI0AwIDYABkQhAAAaAAA"
        "A/wBGNTI1TTI0NUFLTEwKAAAA/ABERUxMIFUyNDEwCiAgAAAA/"
        "QA4TB5REQAKICAgICAgAToCAynxUJAFBAMCBxYBHxITFCAVEQYjCQcHZwMMABAAOC2DAQAA4wUDAQI6gBhxOC1AWCx"
        "FAAZEIQAAHgEdgBhxHBYgWCwlAAZEIQAAngEdAHJR0B4gbihVAAZEIQAAHowK0Iog4C0QED6WAAZEIQAAGAAAAAAAA"
        "AAAAAAAAAAAPg==")
                          // device-id
                          << std::string("xrandr-Dell Inc.-DELL U2410-F525M245AKLL")
                          // name
                          << std::string("DELL U2410")
                          // pnp-id, vendor
                          << std::string("DEL")
                          << QStringList({QStringLiteral("DEL"), QStringLiteral("Dell Inc.")})
                          // serial, eisa-id
                          << std::string("F525M245AKLL")
                          << std::string("")
                          // hash, width, height
                          << std::string("be55eeb5fcc1e775f321c1ae3aa02ef0") << 52U
                          << 32U
                          // gamma
                          << 2.2
                          // colors: rgbw
                          << QQuaternion(1, QVector3D(0.679688, 0.308594, 0))
                          << QQuaternion(1, QVector3D(0.206055, 0.693359, 0))
                          << QQuaternion(1, QVector3D(0.151367, 0.0546875, 0))
                          << QQuaternion(1, QVector3D(0.313477, 0.329102, 0));

    QTest::addRow("samsung") << QByteArray::fromBase64(
        "AP///////wBMLcMFMzJGRQkUAQMOMx14Ku6Ro1RMmSYPUFQjCACBAIFAgYCVAKlAswABAQEBAjqAGHE4LUBYLEUA/"
        "h8RAAAeAAAA/QA4PB5REQAKICAgICAgAAAA/ABTeW5jTWFzdGVyCiAgAAAA/wBIOU1aMzAyMTk2CiAgAC4=")
                             // device-id
                             << std::string("xrandr-Samsung Electric Company-SyncMaster-H9MZ302196")
                             // name
                             << std::string("SyncMaster")
                             // pnp-id, vendor
                             << std::string("SAM")
                             << QStringList({QStringLiteral("SAM"),
                                             QStringLiteral("Samsung Electric Company")})
                             // serial, eisa-id
                             << std::string("H9MZ302196")
                             << std::string("")
                             // hash, width, height
                             << std::string("9384061b2b87ad193f841e07d60e9e1a") << 51U
                             << 29U
                             // gamma
                             << 2.2
                             // colors: rgbw
                             << QQuaternion(1, QVector3D(0.639648, 0.328125, 0))
                             << QQuaternion(1, QVector3D(0.299805, 0.599609, 0))
                             << QQuaternion(1, QVector3D(0.150391, 0.0595703, 0))
                             << QQuaternion(1, QVector3D(0.3125, 0.329102, 0));

    QTest::newRow("sharp") << QByteArray::fromBase64(
        "AP///////"
        "wBNEEoUAAAAAB4ZAQSlHRF4Dt5Qo1RMmSYPUFQAAAABAQEBAQEBAQEBAQEBAQEBzZGAoMAINHAwIDUAJqUQAAAYpHS"
        "AoMAINHAwIDUAJqUQAAAYAAAA/gBSWE40OYFMUTEzM1oxAAAAAAACQQMoABIAAAsBCiAgAMw=")
                           // device-id
                           << std::string("xrandr-Sharp Corporation")
                           // name, unsure why, this screen reports no name
                           << std::string("")
                           // pnp-id, vendor
                           << std::string("SHP")
                           << QStringList(
                                  {QStringLiteral("SHP"), QStringLiteral("Sharp Corporation")})
                           // serial, eisa-id
                           << std::string("")
                           << std::string("RXN49-LQ133Z1")
                           // hash, width, height
                           << std::string("3627c3534e4c82871967b57237bf5b83") << 29U
                           << 17U
                           // gamma
                           << 2.2
                           // colors: rgbw
                           << QQuaternion(1, QVector3D(0.639648, 0.328125, 0))
                           << QQuaternion(1, QVector3D(0.299805, 0.599609, 0))
                           << QQuaternion(1, QVector3D(0.149414, 0.0595703, 0))
                           << QQuaternion(1, QVector3D(0.3125, 0.328125, 0));
}

void TestEdid::testEdidParser()
{
    QFETCH(QByteArray, raw_edid);
    QFETCH(std::string, deviceId);
    QFETCH(std::string, name);
    QFETCH(std::string, pnpId);
    QFETCH(QStringList, vendor);
    QFETCH(std::string, serial);
    QFETCH(std::string, eisaId);
    QFETCH(std::string, hash);
    QFETCH(uint, width);
    QFETCH(uint, height);
    QFETCH(qreal, gamma);
    QFETCH(QQuaternion, red);
    QFETCH(QQuaternion, green);
    QFETCH(QQuaternion, blue);
    QFETCH(QQuaternion, white);

    QScopedPointer<Edid> e(new Edid(raw_edid));
    QCOMPARE(e->isValid(), true);

    // FIXME: we hard-code all deviceIds as xrandr-something, that makes no sense
    QCOMPARE(e->deviceId(), deviceId);
    QCOMPARE(e->name(), name);
    QCOMPARE(e->pnpId(), pnpId);
    // FIXME: needs to return at least the short ID
    // QVERIFY2(vendor.contains(e->vendor()),
    //          qPrintable(QString::fromLatin1("%1 not in list").arg(e->vendor())));
    QCOMPARE(e->serial(), serial);
    QCOMPARE(e->eisaId(), eisaId);
    QCOMPARE(e->hash(), hash);
    QCOMPARE(e->width(), width);
    QCOMPARE(e->height(), height);
    QCOMPARE(e->gamma(), gamma);

    QVERIFY(qFuzzyCompare(e->red(), red));
    QVERIFY(qFuzzyCompare(e->green(), green));
    QVERIFY(qFuzzyCompare(e->blue(), blue));
    QVERIFY(qFuzzyCompare(e->white(), white));
}

QTEST_GUILESS_MAIN(TestEdid)

#include "testedid.moc"
