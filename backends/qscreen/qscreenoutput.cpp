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
#include "qscreenoutput.h"
#include "qscreenbackend.h"

#include "mode.h"
#include "output.h"

#include <QGuiApplication>
#include <QScreen>

using namespace Disman;

QScreenOutput::QScreenOutput(const QScreen* qscreen, QObject* parent)
    : QObject(parent)
    , m_qscreen(qscreen)
    , m_id(-1)
{
}

QScreenOutput::~QScreenOutput()
{
}

int QScreenOutput::id() const
{
    return m_id;
}

void QScreenOutput::setId(const int newId)
{
    m_id = newId;
}

const QScreen* QScreenOutput::qscreen() const
{
    return m_qscreen;
}

std::string QScreenOutput::description() const
{
    std::string ret;

    if (auto vendor = m_qscreen->manufacturer().toStdString(); vendor.size()) {
        ret += vendor + " ";
    }
    if (auto model = m_qscreen->model().toStdString(); model.size()) {
        ret += model + " ";
    }

    if (!ret.size()) {
        return m_qscreen->name().toStdString();
    }
    return ret + "(" + m_qscreen->name().toStdString() + ")";
}

OutputPtr QScreenOutput::toDismanOutput() const
{
    OutputPtr output(new Output);
    output->setId(m_id);
    output->set_name(m_qscreen->name().toStdString());

    auto const descr = description();
    output->set_description(descr);
    output->set_hash(descr);

    updateDismanOutput(output);
    return output;
}

void QScreenOutput::updateDismanOutput(OutputPtr& output) const
{
    output->setEnabled(true);

    // Rotation - translate QScreen::primaryOrientation() to Output::rotation()
    if (m_qscreen->primaryOrientation() == Qt::PortraitOrientation) {
        // 90 degrees
        output->setRotation(Output::Right);
    } else if (m_qscreen->primaryOrientation() == Qt::InvertedLandscapeOrientation) {
        // 180 degrees
        output->setRotation(Output::Inverted);
    } else if (m_qscreen->primaryOrientation() == Qt::InvertedPortraitOrientation) {
        // 270 degrees
        output->setRotation(Output::Left);
    }

    // Physical size, geometry, etc.
    QSize mm;
    qreal physicalWidth;
    physicalWidth = m_qscreen->size().width() / (m_qscreen->physicalDotsPerInchX() / 25.4);
    mm.setWidth(qRound(physicalWidth));
    qreal physicalHeight;
    physicalHeight = m_qscreen->size().height() / (m_qscreen->physicalDotsPerInchY() / 25.4);
    mm.setHeight(qRound(physicalHeight));
    output->setSizeMm(mm);
    output->setPosition(m_qscreen->availableGeometry().topLeft());

    // Modes: we create a single default mode and go with that
    ModePtr mode(new Mode);
    std::string const modeid = "defaultmode";
    mode->setId(modeid);
    mode->setRefreshRate(m_qscreen->refreshRate());
    mode->setSize(m_qscreen->size());

    const QString modename = QString::number(m_qscreen->size().width()) + QLatin1String("x")
        + QString::number(m_qscreen->size().height()) + QLatin1String("@")
        + QString::number(m_qscreen->refreshRate());
    mode->setName(modename.toStdString());

    ModeList modes;
    modes[modeid] = mode;
    output->setModes(modes);
    output->set_mode(mode);
}
