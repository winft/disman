/*************************************************************************************
 *  Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *  Copyright     2012 by Sebastian Kügler <sebas@kde.org>                           *
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
#include <QElapsedTimer>
#include <QObject>
#include <QtTest>

#include "backendmanager_p.h"
#include "config.h"
#include "getconfigoperation.h"
#include "mode.h"
#include "output.h"

Q_LOGGING_CATEGORY(DISMAN_QSCREEN, "disman.qscreen")

using namespace Disman;

class testQScreenBackend : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void verifyConfig();
    void verifyScreen();
    void verifyOutputs();
    void verifyModes();
    void verifyFeatures();
    void commonUsagePattern();
    void cleanupTestCase();

private:
    QProcess m_process;
    ConfigPtr m_config;
    QString m_backend;
};

void testQScreenBackend::initTestCase()
{
    qputenv("DISMAN_LOGGING", "false");
    qputenv("DISMAN_BACKEND", "qscreen");
    qputenv("DISMAN_IN_PROCESS", "1");
    Disman::BackendManager::instance()->shutdown_backend();

    m_backend = QString::fromLocal8Bit(qgetenv("DISMAN_BACKEND"));

    QElapsedTimer t;
    t.start();
    auto* op = new GetConfigOperation();
    op->exec();
    m_config = op->config();
    const int n = t.nsecsElapsed();
    qDebug() << "Test took: " << n << "ns";
}

void testQScreenBackend::verifyConfig()
{
    QVERIFY(m_config);
    if (!m_config) {
        QSKIP("QScreenbackend invalid", SkipAll);
    }
}

void testQScreenBackend::verifyScreen()
{
    ScreenPtr screen = m_config->screen();

    QVERIFY(screen->min_size().width() <= screen->max_size().width());
    QVERIFY(screen->min_size().height() <= screen->max_size().height());

    QVERIFY(screen->min_size().width() <= screen->current_size().width());
    QVERIFY(screen->min_size().height() <= screen->current_size().height());

    QVERIFY(screen->max_size().width() >= screen->current_size().width());
    QVERIFY(screen->max_size().height() >= screen->current_size().height());
    QVERIFY(m_config->screen()->max_outputs_count() > 0);
}

void testQScreenBackend::verifyOutputs()
{
    QVERIFY(m_config->primary_output());
    if (m_backend == QLatin1String("screen")) {
        QCOMPARE(m_config->outputs().size(), QGuiApplication::screens().size());
    }

    const Disman::OutputPtr primary = m_config->primary_output();
    QVERIFY(primary->enabled());
    // qDebug() << "Primary geometry? " << primary->geometry();
    // qDebug() << " prim modes: " << primary->modes();

    QList<int> ids;
    for (auto const& [key, output] : m_config->outputs()) {
        qDebug() << " _____________________ Output: " << output;
        qDebug() << "   output name: " << output->name().c_str();
        qDebug() << "   output modes: " << output->modes().size();
        for (auto const& [key, mode] : output->modes()) {
            qDebug() << "      " << mode;
        }
        qDebug() << "   output enabled: " << output->enabled();
        qDebug() << "   output physical_size : " << output->physical_size();
        QVERIFY(output->name().size());
        QVERIFY(output->id() > -1);
        QVERIFY(output->enabled());
        QVERIFY(output->geometry() != QRectF(1, 1, 1, 1));
        QVERIFY(output->geometry() != QRectF());

        // Pass, but leave a note, when the x server doesn't report physical size
        if (!output->physical_size().isValid()) {
            QEXPECT_FAIL(
                "", "The X server doesn't return a sensible physical output size", Continue);
            QVERIFY(output->physical_size() != QSize());
        }
        QCOMPARE(output->rotation(), Output::None);
        QVERIFY(!ids.contains(output->id()));
        ids << output->id();
    }
}

void testQScreenBackend::verifyModes()
{
    const Disman::OutputPtr primary = m_config->primary_output();
    QVERIFY(primary);
    QVERIFY(primary->modes().size() > 0);

    for (auto const& [output_key, output] : m_config->outputs()) {
        for (auto const& [mode_key, mode] : output->modes()) {
            qDebug() << "   Mode   : " << mode->name().c_str();
            QVERIFY(!mode->name().empty());
            QVERIFY(mode->refresh() > 0);
            QVERIFY(mode->size() != QSize());
        }
    }
}

void testQScreenBackend::commonUsagePattern()
{
    auto* op = new GetConfigOperation();
    op->exec();

    const Disman::OutputList outputs = op->config()->outputs();

    QVariantList outputList;
    for (auto const& [key, output] : outputs) {
        QVariantMap info;
        info[QStringLiteral("id")] = output->id();
        info[QStringLiteral("enabled")] = output->enabled();
        info[QStringLiteral("rotation")] = output->rotation();

        QVariantMap pos;
        pos[QStringLiteral("x")] = output->position().x();
        pos[QStringLiteral("y")] = output->position().y();
        info[QStringLiteral("pos")] = pos;

        if (output->enabled()) {
            const Disman::ModePtr mode = output->auto_mode();
            if (!mode) {
                // qWarning() << "CurrentMode is null" << output->name();
                return;
            }

            QVariantMap modeInfo;
            modeInfo[QStringLiteral("refresh")] = mode->refresh();

            QVariantMap modeSize;
            modeSize[QStringLiteral("width")] = mode->size().width();
            modeSize[QStringLiteral("height")] = mode->size().height();
            modeInfo[QStringLiteral("size")] = modeSize;

            info[QStringLiteral("mode")] = modeInfo;
        }

        outputList.append(info);
    }
}

void testQScreenBackend::cleanupTestCase()
{
    Disman::BackendManager::instance()->shutdown_backend();
    qApp->exit(0);
}

void testQScreenBackend::verifyFeatures()
{
    GetConfigOperation* op = new GetConfigOperation();
    op->exec();
    auto config = op->config();
    QVERIFY(config->supported_features().testFlag(Config::Feature::None));
    QVERIFY(!config->supported_features().testFlag(Config::Feature::Writable));
    QVERIFY(!config->supported_features().testFlag(Config::Feature::PrimaryDisplay));
}

QTEST_MAIN(testQScreenBackend)

#include "testqscreenbackend.moc"
