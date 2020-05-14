/*************************************************************************
Copyright © 2014-2015 Sebastian Kügler <sebas@kde.org>
Copyright © 2019-2020 Roman Gilg <subdiff@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
**************************************************************************/
#include "waylandoutput.h"
#include "../utils.h"

using namespace Disman;

WaylandOutput::WaylandOutput(quint32 id, QObject *parent)
    : QObject(parent)
    , m_id(id)
{
}

quint32 WaylandOutput::id() const
{
    return m_id;
}

OutputPtr WaylandOutput::toDismanOutput()
{
    OutputPtr output(new Output());
    output->setId(m_id);
    updateDismanOutput(output);
    return output;
}

Disman::Output::Type WaylandOutput::guessType(const QString &type, const QString &name) const
{
    return Utils::guessOutputType(type, name);
}
