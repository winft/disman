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

#include "abstractbackend.h"

#include <QEventLoop>
#include <QPointer>

#include <memory>
#include <vector>

class KPluginMetaData;

namespace Disman
{
class Filer_controller;
class WaylandInterface;
class WaylandOutput;
class WaylandScreen;

class WaylandBackend : public Disman::AbstractBackend
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kwinft.disman.backends.wayland")

public:
    explicit WaylandBackend();
    ~WaylandBackend() override;

    QString name() const override;
    QString serviceName() const override;
    bool isValid() const override;

    Disman::ConfigPtr config() const override;
    void setConfig(const Disman::ConfigPtr& config) override;

    QByteArray edid(int outputId) const override;

    QMap<int, WaylandOutput*> outputMap() const;

private:
    struct PendingInterface {
        bool operator==(const PendingInterface& other)
        {
            return name == other.name;
        }
        QString name;
        WaylandInterface* interface;
        QThread* thread;
    };

    void initKWinTabletMode();
    void setScreenOutputs();

    void queryInterfaces();
    void queryInterface(KPluginMetaData* plugin);
    void takeInterface(const PendingInterface& pending);
    void rejectInterface(const PendingInterface& pending);

    bool set_config_impl(Disman::ConfigPtr const& config);

    std::unique_ptr<Filer_controller> m_filer_controller;

    Disman::ConfigPtr m_config{nullptr};
    std::unique_ptr<WaylandScreen> m_screen;
    QPointer<WaylandInterface> m_interface;
    QThread* m_thread{nullptr};

    bool m_tabletModeAvailable{false};
    bool m_tabletModeEngaged{false};

    QEventLoop m_syncLoop;

    std::vector<PendingInterface> m_pendingInterfaces;
};

}
