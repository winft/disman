/*************************************************************************************
 *  Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *  Copyright (C) 2012, 2013 by Daniel Vrátil <dvratil@redhat.com>                   *
 *  Copyright 2014-2015 Sebastian Kügler <sebas@kde.org>                             *
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
#pragma once

#include "../backend_impl.h"

#include <QEventLoop>
#include <QPointer>

#include <memory>
#include <vector>

class QThread;

namespace Disman
{

class WaylandInterface;
class WaylandOutput;
class WaylandScreen;

class WaylandBackend : public Disman::BackendImpl
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kwinft.disman.backends.wayland")

public:
    explicit WaylandBackend();
    ~WaylandBackend() override;

    QString name() const override;
    QString service_name() const override;
    bool valid() const override;

    void update_config(ConfigPtr& config) const override;
    bool set_config_system(Disman::ConfigPtr const& config) override;

    std::map<int, WaylandOutput*> outputMap() const;

private:
    void initKWinTabletMode();
    void setScreenOutputs();

    void queryInterface();

    std::unique_ptr<WaylandScreen> m_screen;
    std::unique_ptr<WaylandInterface> m_interface;
    QThread* m_thread{nullptr};

    struct {
        bool available{false};
        bool engaged{false};
    } tablet_mode;

    QEventLoop m_syncLoop;
};

}
