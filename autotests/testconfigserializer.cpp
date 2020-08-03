/*
 * Copyright (C) 2014  Daniel Vratil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#include <QObject>
#include <QtTest>

#include "../src/configserializer_p.h"
#include "../src/mode.h"
#include "../src/output.h"
#include "../src/screen.h"
#include "../src/types.h"

class TestConfigSerializer : public QObject
{
    Q_OBJECT

public:
    TestConfigSerializer()
    {
    }

private Q_SLOTS:
    void testSerializePoint()
    {
        const QPoint point(42, 24);

        const QJsonObject obj = Disman::ConfigSerializer::serializePoint(point);
        QVERIFY(!obj.isEmpty());

        QCOMPARE(obj[QLatin1String("x")].toInt(), point.x());
        QCOMPARE(obj[QLatin1String("y")].toInt(), point.y());
    }

    void testSerializeSize()
    {
        const QSize size(800, 600);

        const QJsonObject obj = Disman::ConfigSerializer::serializeSize(size);
        QVERIFY(!obj.isEmpty());

        QCOMPARE(obj[QLatin1String("width")].toInt(), size.width());
        QCOMPARE(obj[QLatin1String("height")].toInt(), size.height());
    }

    void testSerializeList()
    {
        QStringList stringList;
        stringList << QStringLiteral("Item 1") << QStringLiteral("Item 2")
                   << QStringLiteral("Item 3") << QStringLiteral("Item 4");

        QJsonArray arr = Disman::ConfigSerializer::serializeList<QString>(stringList);
        QCOMPARE(arr.size(), stringList.size());

        for (int i = 0; i < arr.size(); ++i) {
            QCOMPARE(arr.at(i).toString(), stringList.at(i));
        }

        QList<int> intList;
        intList << 4 << 3 << 2 << 1;

        arr = Disman::ConfigSerializer::serializeList<int>(intList);
        QCOMPARE(arr.size(), intList.size());

        for (int i = 0; i < arr.size(); ++i) {
            QCOMPARE(arr.at(i).toInt(), intList[i]);
        }
    }

    void testSerializeScreen()
    {
        Disman::ScreenPtr screen(new Disman::Screen);
        screen->setId(12);
        screen->setMinSize(QSize(360, 360));
        screen->setMaxSize(QSize(8192, 8192));
        screen->setCurrentSize(QSize(3600, 1280));
        screen->setMaxActiveOutputsCount(3);

        const QJsonObject obj = Disman::ConfigSerializer::serializeScreen(screen);
        QVERIFY(!obj.isEmpty());

        QCOMPARE(obj[QLatin1String("id")].toInt(), screen->id());
        QCOMPARE(obj[QLatin1String("maxActiveOutputsCount")].toInt(),
                 screen->maxActiveOutputsCount());
        const QJsonObject minSize = obj[QLatin1String("minSize")].toObject();
        QCOMPARE(minSize[QLatin1String("width")].toInt(), screen->minSize().width());
        QCOMPARE(minSize[QLatin1String("height")].toInt(), screen->minSize().height());
        const QJsonObject maxSize = obj[QLatin1String("maxSize")].toObject();
        QCOMPARE(maxSize[QLatin1String("width")].toInt(), screen->maxSize().width());
        QCOMPARE(maxSize[QLatin1String("height")].toInt(), screen->maxSize().height());
        const QJsonObject currSize = obj[QLatin1String("currentSize")].toObject();
        QCOMPARE(currSize[QLatin1String("width")].toInt(), screen->currentSize().width());
        QCOMPARE(currSize[QLatin1String("height")].toInt(), screen->currentSize().height());
    }

    void testSerializeMode()
    {
        Disman::ModePtr mode(new Disman::Mode);
        mode->setId(QStringLiteral("755"));
        mode->setName(QStringLiteral("1280x1024"));
        mode->setRefreshRate(50.666);
        mode->setSize(QSize(1280, 1024));

        const QJsonObject obj = Disman::ConfigSerializer::serializeMode(mode);
        QVERIFY(!obj.isEmpty());

        QCOMPARE(obj[QLatin1String("id")].toString(), mode->id());
        QCOMPARE(obj[QLatin1String("name")].toString(), mode->name());
        QCOMPARE((float)obj[QLatin1String("refreshRate")].toDouble(), mode->refreshRate());
        const QJsonObject size = obj[QLatin1String("size")].toObject();
        QCOMPARE(size[QLatin1String("width")].toInt(), mode->size().width());
        QCOMPARE(size[QLatin1String("height")].toInt(), mode->size().height());
    }

    void testSerializeOutput()
    {
        Disman::ModeList modes;
        Disman::ModePtr mode(new Disman::Mode);
        mode->setId(QStringLiteral("1"));
        mode->setName(QStringLiteral("800x600"));
        mode->setSize(QSize(800, 600));
        mode->setRefreshRate(50.4);
        modes.insert(mode->id(), mode);

        Disman::OutputPtr output(new Disman::Output);
        output->setId(60);
        output->setName(QStringLiteral("LVDS-0"));
        output->setType(Disman::Output::Panel);
        output->setIcon(QString());
        output->setModes(modes);
        output->setPosition(QPoint(1280, 0));
        output->setRotation(Disman::Output::None);
        output->setPreferredModes(QStringList() << QStringLiteral("1"));
        output->setEnabled(true);
        output->setPrimary(true);
        output->setSizeMm(QSize(310, 250));

        const QJsonObject obj = Disman::ConfigSerializer::serializeOutput(output);
        QVERIFY(!obj.isEmpty());

        QCOMPARE(obj[QLatin1String("id")].toInt(), output->id());
        QCOMPARE(obj[QLatin1String("name")].toString(), output->name());
        QCOMPARE(static_cast<Disman::Output::Type>(obj[QLatin1String("type")].toInt()),
                 output->type());
        QCOMPARE(obj[QLatin1String("icon")].toString(), output->icon());
        const QJsonArray arr = obj[QLatin1String("modes")].toArray();
        QCOMPARE(arr.size(), output->modes().count());

        QJsonObject pos = obj[QLatin1String("position")].toObject();
        QCOMPARE(pos[QLatin1String("x")].toInt(), output->position().x());
        QCOMPARE(pos[QLatin1String("y")].toInt(), output->position().y());

        QCOMPARE(static_cast<Disman::Output::Rotation>(obj[QLatin1String("rotation")].toInt()),
                 output->rotation());

        QVERIFY(!obj.contains(QStringLiteral("resolution")));
        QVERIFY(!obj.contains(QStringLiteral("refresh_rate")));

        QCOMPARE(obj[QLatin1String("enabled")].toBool(), output->isEnabled());
        QCOMPARE(obj[QLatin1String("primary")].toBool(), output->isPrimary());
        const QJsonObject sizeMm = obj[QLatin1String("sizeMM")].toObject();
        QCOMPARE(sizeMm[QLatin1String("width")].toInt(), output->sizeMm().width());
        QCOMPARE(sizeMm[QLatin1String("height")].toInt(), output->sizeMm().height());

        output->set_mode(mode);

        auto const obj2 = Disman::ConfigSerializer::serializeOutput(output);

        auto const res = obj2[QStringLiteral("resolution")].toObject();
        QCOMPARE(res[QStringLiteral("width")].toInt(), output->commanded_mode()->size().width());
        QCOMPARE(res[QStringLiteral("height")].toInt(), output->commanded_mode()->size().height());

        QCOMPARE(obj2[QStringLiteral("refresh_rate")].toDouble(),
                 output->commanded_mode()->refreshRate());
    }
};

QTEST_MAIN(TestConfigSerializer)

#include "testconfigserializer.moc"
