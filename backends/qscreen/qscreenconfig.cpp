/*************************************************************************************
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
#include "qscreenconfig.h"
#include "qscreenbackend.h"
#include "qscreenoutput.h"
#include "qscreenscreen.h"

#include "qscreen_logging.h"

#include "mode.h"
#include "output.h"

#include <QGuiApplication>
#include <QRect>
#include <QScreen>

using namespace Disman;

QScreenConfig::QScreenConfig(QObject* parent)
    : QObject(parent)
    , m_screen(new QScreenScreen(this))
    , m_blockSignals(true)
{
    foreach (const QScreen* qscreen, QGuiApplication::screens()) {
        screenAdded(qscreen);
    }
    m_blockSignals = false;
    connect(qApp, &QGuiApplication::screenAdded, this, &QScreenConfig::screenAdded);
    connect(qApp, &QGuiApplication::screenRemoved, this, &QScreenConfig::screenRemoved);
}

QScreenConfig::~QScreenConfig()
{
    for (auto& [key, out] : m_outputMap) {
        delete out;
    }
}

int QScreenConfig::outputId(const QScreen* qscreen)
{
    QList<int> ids;
    for (auto& [key, output] : m_outputMap) {
        if (qscreen == output->qscreen()) {
            return output->id();
        }
    }
    m_lastOutputId++;
    return m_lastOutputId;
}

void QScreenConfig::screenAdded(const QScreen* qscreen)
{
    qCDebug(DISMAN_QSCREEN) << "Screen added" << qscreen << qscreen->name();
    QScreenOutput* qscreenoutput = new QScreenOutput(qscreen, this);
    qscreenoutput->set_id(outputId(qscreen));
    m_outputMap.insert({qscreenoutput->id(), qscreenoutput});

    if (!m_blockSignals) {
        Q_EMIT config_changed();
    }
}

void QScreenConfig::screenRemoved(QScreen* qscreen)
{
    qCDebug(DISMAN_QSCREEN) << "Screen removed" << qscreen << QGuiApplication::screens().count();
    // Find output matching the QScreen object and remove it
    int removedOutputId = -1;
    for (auto& [key, output] : m_outputMap) {
        if (output->qscreen() == qscreen) {
            removedOutputId = output->id();
            m_outputMap.erase(removedOutputId);
            delete output;
        }
    }
    Q_EMIT config_changed();
}

void QScreenConfig::update_config(ConfigPtr& config) const
{
    config->setScreen(m_screen->toDismanScreen());

    // Removing removed outputs.
    for (auto const& [key, output] : config->outputs()) {
        if (m_outputMap.find(output->id()) == m_outputMap.end()) {
            config->remove_output(output->id());
        }
    }

    // Add Disman::Outputs that aren't in the list yet.
    auto dismanOutputs = config->outputs();

    for (auto& [key, output] : m_outputMap) {
        Disman::OutputPtr dismanOutput;

        auto it = dismanOutputs.find(output->id());
        if (it == dismanOutputs.end()) {
            dismanOutput = output->toDismanOutput();
            dismanOutputs.insert({dismanOutput->id(), dismanOutput});
        } else {
            dismanOutput = it->second;
        }

        output->updateDismanOutput(dismanOutput);
        if (QGuiApplication::primaryScreen() == output->qscreen()) {
            config->set_primary_output(dismanOutput);
        }
    }

    config->set_outputs(dismanOutputs);
}

std::map<int, QScreenOutput*> QScreenConfig::outputMap() const
{
    return m_outputMap;
}
