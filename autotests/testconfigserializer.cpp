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

#include "configserializer_p.h"
#include "mode.h"
#include "output.h"
#include "screen.h"
#include "types.h"

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

        const QJsonObject obj = Disman::ConfigSerializer::serialize_point(point);
        QVERIFY(!obj.isEmpty());

        QCOMPARE(obj[QLatin1String("x")].toInt(), point.x());
        QCOMPARE(obj[QLatin1String("y")].toInt(), point.y());
    }

    void testSerializeSize()
    {
        const QSize size(800, 600);

        const QJsonObject obj = Disman::ConfigSerializer::serialize_size(size);
        QVERIFY(!obj.isEmpty());

        QCOMPARE(obj[QLatin1String("width")].toInt(), size.width());
        QCOMPARE(obj[QLatin1String("height")].toInt(), size.height());
    }

    void testSerializeList()
    {
        QStringList stringList;
        stringList << QStringLiteral("Item 1") << QStringLiteral("Item 2")
                   << QStringLiteral("Item 3") << QStringLiteral("Item 4");

        QJsonArray arr = Disman::ConfigSerializer::serialize_list<QString>(stringList);
        QCOMPARE(arr.size(), stringList.size());

        for (int i = 0; i < arr.size(); ++i) {
            QCOMPARE(arr.at(i).toString(), stringList.at(i));
        }

        QList<int> intList;
        intList << 4 << 3 << 2 << 1;

        arr = Disman::ConfigSerializer::serialize_list<int>(intList);
        QCOMPARE(arr.size(), intList.size());

        for (int i = 0; i < arr.size(); ++i) {
            QCOMPARE(arr.at(i).toInt(), intList[i]);
        }
    }

    void testSerializeScreen()
    {
        Disman::ScreenPtr screen(new Disman::Screen);
        screen->set_id(12);
        screen->set_min_size(QSize(360, 360));
        screen->set_max_size(QSize(8192, 8192));
        screen->set_current_size(QSize(3600, 1280));
        screen->set_max_outputs_count(3);

        const QJsonObject obj = Disman::ConfigSerializer::serialize_screen(screen);
        QVERIFY(!obj.isEmpty());

        QCOMPARE(obj[QLatin1String("id")].toInt(), screen->id());
        QCOMPARE(obj[QLatin1String("max_outputs_count")].toInt(), screen->max_outputs_count());
        const QJsonObject min_size = obj[QLatin1String("min_size")].toObject();
        QCOMPARE(min_size[QLatin1String("width")].toInt(), screen->min_size().width());
        QCOMPARE(min_size[QLatin1String("height")].toInt(), screen->min_size().height());
        const QJsonObject max_size = obj[QLatin1String("max_size")].toObject();
        QCOMPARE(max_size[QLatin1String("width")].toInt(), screen->max_size().width());
        QCOMPARE(max_size[QLatin1String("height")].toInt(), screen->max_size().height());
        const QJsonObject currSize = obj[QLatin1String("current_size")].toObject();
        QCOMPARE(currSize[QLatin1String("width")].toInt(), screen->current_size().width());
        QCOMPARE(currSize[QLatin1String("height")].toInt(), screen->current_size().height());
    }

    void testSerializeMode()
    {
        Disman::ModePtr mode(new Disman::Mode);
        mode->set_id("755");
        mode->set_name("1280x1024");
        mode->set_refresh(50.666);
        mode->set_size(QSize(1280, 1024));

        const QJsonObject obj = Disman::ConfigSerializer::serialize_mode(mode);
        QVERIFY(!obj.isEmpty());

        QCOMPARE(obj[QLatin1String("id")].toString().toStdString(), mode->id());
        QCOMPARE(obj[QLatin1String("name")].toString().toStdString(), mode->name());
        QCOMPARE(obj[QLatin1String("refresh")].toDouble(), mode->refresh());
        const QJsonObject size = obj[QLatin1String("size")].toObject();
        QCOMPARE(size[QLatin1String("width")].toInt(), mode->size().width());
        QCOMPARE(size[QLatin1String("height")].toInt(), mode->size().height());
    }

    void testSerializeOutput()
    {
        Disman::ModeList modes;
        Disman::ModePtr mode(new Disman::Mode);
        mode->set_id("1");
        mode->set_name("800x600");
        mode->set_size(QSize(800, 600));
        mode->set_refresh(50.4);
        modes.insert({mode->id(), mode});

        Disman::OutputPtr output(new Disman::Output);
        output->set_id(60);
        output->set_name("LVDS-0");
        output->setType(Disman::Output::Panel);
        output->set_modes(modes);
        output->set_position(QPoint(1280, 0));
        output->set_rotation(Disman::Output::None);
        output->set_preferred_modes({"1"});
        output->set_enabled(true);
        output->set_physical_size(QSize(310, 250));

        const QJsonObject obj = Disman::ConfigSerializer::serialize_output(output);
        QVERIFY(!obj.isEmpty());

        QCOMPARE(obj[QLatin1String("id")].toInt(), output->id());
        QCOMPARE(obj[QLatin1String("name")].toString(), QString::fromStdString(output->name()));
        QCOMPARE(static_cast<Disman::Output::Type>(obj[QLatin1String("type")].toInt()),
                 output->type());
        const QJsonArray arr = obj[QLatin1String("modes")].toArray();
        QCOMPARE(arr.size(), output->modes().size());

        QJsonObject pos = obj[QLatin1String("position")].toObject();
        QCOMPARE(pos[QLatin1String("x")].toInt(), output->position().x());
        QCOMPARE(pos[QLatin1String("y")].toInt(), output->position().y());

        QCOMPARE(static_cast<Disman::Output::Rotation>(obj[QLatin1String("rotation")].toInt()),
                 output->rotation());

        QVERIFY(!obj.contains(QStringLiteral("resolution")));
        QVERIFY(!obj.contains(QStringLiteral("refresh")));

        QCOMPARE(obj[QLatin1String("enabled")].toBool(), output->enabled());
        const QJsonObject physical_size = obj[QLatin1String("physical_size")].toObject();
        QCOMPARE(physical_size[QLatin1String("width")].toInt(), output->physical_size().width());
        QCOMPARE(physical_size[QLatin1String("height")].toInt(), output->physical_size().height());

        output->set_mode(mode);

        auto const obj2 = Disman::ConfigSerializer::serialize_output(output);

        auto const res = obj2[QStringLiteral("resolution")].toObject();
        QCOMPARE(res[QStringLiteral("width")].toInt(), output->commanded_mode()->size().width());
        QCOMPARE(res[QStringLiteral("height")].toInt(), output->commanded_mode()->size().height());

        QCOMPARE(obj2[QStringLiteral("refresh")].toDouble(), output->commanded_mode()->refresh());
    }
};

QTEST_MAIN(TestConfigSerializer)

#include "testconfigserializer.moc"
