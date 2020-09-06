/*************************************************************************************
 *  Copyright 2014  Daniel Vrátil <dvratil@redhat.com>                               *
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
#ifndef DISMAN_TYPES_H
#define DISMAN_TYPES_H

#include <QMap>
#include <QSharedPointer>

#include <map>
#include <string>

namespace Disman
{

class Config;
typedef QSharedPointer<Disman::Config> ConfigPtr;
class Screen;
typedef QSharedPointer<Disman::Screen> ScreenPtr;
class Output;
typedef QSharedPointer<Disman::Output> OutputPtr;
typedef std::map<int, Disman::OutputPtr> OutputList;

class Mode;
typedef QSharedPointer<Disman::Mode> ModePtr;
typedef std::map<std::string, Disman::ModePtr> ModeList;

}

#endif
