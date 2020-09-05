/*************************************************************************************
 *  Copyright 2013 Alejandro Fiestas Olivares <afiestas@kde.org>                  *
 *  Copyright 2014 Sebastian Kügler <sebas@kde.org>                                  *
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
#include "testpnp.h"

#include "config.h"
#include "configmonitor.h"
#include "getconfigoperation.h"
#include "mode.h"
#include "output.h"
#include "screen.h"

#include <QGuiApplication>
#include <QRect>
#include <QScreen>

using namespace Disman;

QString typetoString(const Output::Type& type)
{
    switch (type) {
    case Output::Unknown:
        return QStringLiteral("Unknown");
    case Output::Panel:
        return QStringLiteral("Panel (Laptop)");
    case Output::VGA:
        return QStringLiteral("VGA");
    case Output::DVII:
        return QStringLiteral("DVI-I");
    case Output::DVIA:
        return QStringLiteral("DVI-A");
    case Output::DVID:
        return QStringLiteral("DVI-D");
    case Output::HDMI:
        return QStringLiteral("HDMI");
    case Output::TV:
        return QStringLiteral("TV");
    case Output::TVComposite:
        return QStringLiteral("TV-Composite");
    case Output::TVSVideo:
        return QStringLiteral("TV-SVideo");
    case Output::TVComponent:
        return QStringLiteral("TV-Component");
    case Output::TVSCART:
        return QStringLiteral("TV-SCART");
    case Output::TVC4:
        return QStringLiteral("TV-C4");
    case Output::DisplayPort:
        return QStringLiteral("DisplayPort");
    default:
        return QStringLiteral("Invalid Type");
    };
}

TestPnp::TestPnp(bool monitor, QObject* parent)
    : QObject(parent)
    , m_monitor(monitor)
{
    init();
}

TestPnp::~TestPnp()
{
}

void TestPnp::init()
{
    connect(new Disman::GetConfigOperation(),
            &Disman::GetConfigOperation::finished,
            this,
            &TestPnp::configReady);
}

void TestPnp::configReady(Disman::ConfigOperation* op)
{
    m_config = qobject_cast<Disman::GetConfigOperation*>(op)->config();
    if (!m_config) {
        qDebug() << "Config is invalid, probably backend couldn't load";
        qApp->quit();
        return;
    }
    if (!m_config->screen()) {
        qDebug() << "No screen in the configuration, broken backend";
        qApp->quit();
        return;
    }

    print();
    if (m_monitor) {
        ConfigMonitor::instance()->addConfig(m_config);
    } else {
        qApp->quit();
    }
}

void TestPnp::print()
{
    qDebug() << "Screen:";
    qDebug() << "\tmaxSize:" << m_config->screen()->maxSize();
    qDebug() << "\tminSize:" << m_config->screen()->minSize();
    qDebug() << "\tcurrentSize:" << m_config->screen()->currentSize();

    qDebug() << "Primary:"
             << (m_config->primaryOutput() ? std::to_string(m_config->primaryOutput()->id()).c_str()
                                           : "none");

    const OutputList outputs = m_config->outputs();
    Q_FOREACH (const OutputPtr& output, outputs) {
        qDebug() << "\n-----------------------------------------------------\n";
        qDebug() << "Id: " << output->id();
        qDebug() << "Name: " << output->name().c_str();
        qDebug() << "Type: " << typetoString(output->type());
        qDebug() << "Enabled: " << output->isEnabled();
        qDebug() << "Rotation: " << output->rotation();
        qDebug() << "Pos: " << output->position();
        qDebug() << "MMSize: " << output->sizeMm();
        if (output->auto_mode()) {
            qDebug() << "Size: " << output->auto_mode()->size();
        }
        qDebug() << "Mode: " << output->auto_mode()->id().c_str();
        qDebug() << "Preferred Mode: " << output->preferred_mode()->id().c_str();
        qDebug() << "Preferred modes: ";
        for (auto const& mode_string : output->preferredModes()) {
            qDebug() << "\t" << mode_string.c_str();
        }

        qDebug() << "Modes: ";
        for (auto const& [key, mode] : output->modes()) {
            qDebug() << "\t" << mode->id().c_str() << "  " << mode->name().c_str() << " "
                     << mode->size() << " " << mode->refreshRate();
        }
    }
}
