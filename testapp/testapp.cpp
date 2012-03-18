/*************************************************************************************
 *  Copyright (C) 2011 by Alex Fiestas <afiestas@kde.org>                            *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU General Public License                      *
 *  as published by the Free Software Foundation; either version 2                   *
 *  of the License, or (at your option) any later version.                           *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA   *
 *************************************************************************************/

#include <QtGui/QApplication>
#include <QtCore/QtDebug>

#include "../src/lib/xrandr.h"
#include "../src/lib/screen.h"
#include "../src/lib/crtc.h"
#include "../src/lib/output.h"
#include "../src/lib/mode.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    XRandR *randr =  XRandR::self();

    if (!randr) {
        return -1;
    }

    QPair<int,int> version =  randr->version();
    qDebug() << "Xrandr version: " << version.first << "." << version.second;

    QList<QRandR::Screen *> screens = randr->screens();
    qDebug() << "You have " << screens.count() << "screens(s) in your display. Information:";

    Q_FOREACH(QRandR::Screen *screen, screens) {
        const QSize minSize = screen->minSize();
        const QSize maxSize = screen->maxSize();
        const QSize currentSize = screen->currentSize();

        qDebug() << "MinSize: " << minSize.width() << "x" << minSize.height()
                 << "MaxSize: " << maxSize.width() << "x" << maxSize.height()
                 << "CurrentSize: " << currentSize.width() << "x" << currentSize.height();

        QHash <RROutput, QRandR::Crtc* > crtcList = screen->crtc();

        qDebug() << "Num of CRTC: " << crtcList.count();
        Q_FOREACH(QRandR::Crtc *crtc, crtcList) {
            qDebug() << "\tChecking CRTC: " << crtc->id();
            qDebug() << "\t\tRect: " << crtc->rect();
        }

        QHash <RROutput, QRandR::Output* > outputList = screen->outputs();

        qDebug() << "Num of Output: " << outputList.count();
        Q_FOREACH(QRandR::Output *output, outputList) {
            qDebug() << "\tChecking Output: " << output->id();
            qDebug() << "\t\tName: " << output->name();
            qDebug() << "\t\tEnabled: " << output->isEnabled();
            qDebug() << "\t\tConnected: " << output->isConnected();
            qDebug();

            QHash <RRMode, QRandR::Mode* > modesList = output->modes();

            qDebug() << "\t\tNum of Modes: " << modesList.count();
            Q_FOREACH(QRandR::Mode *mode, modesList) {
                qDebug() << "\t\t\t" << mode->name() << "\t" << mode->rate();
            }
        }

        qDebug();
        QHash<RRMode, QRandR::Mode*> modeList = screen->modes();
        qDebug() << "Num of Modes: " << modeList.count();
        Q_FOREACH(QRandR::Mode *mode, modeList) {
            qDebug() << "\t" << mode->name() << "\t" << mode->rate();
        }
    }

//     return app.exec();
}
