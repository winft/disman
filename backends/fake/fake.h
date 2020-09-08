/*************************************************************************************
 *  Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
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
#ifndef FAKE_BACKEND_H
#define FAKE_BACKEND_H

#include "backend_impl.h"

#include <QLoggingCategory>
#include <QObject>
#include <memory>

namespace Disman
{
class Filer_controller;
}

class Fake : public Disman::BackendImpl
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kf5.disman.backends.fake")

public:
    explicit Fake();
    ~Fake() override;

    void init(const QVariantMap& arguments) override;

    QString name() const override;
    QString service_name() const override;
    Disman::ConfigPtr config() const override;
    void set_config(const Disman::ConfigPtr& config) override;
    bool valid() const override;

    void setEnabled(int outputId, bool enabled);
    void setPrimary(int outputId, bool primary);
    void setCurrentModeId(int outputId, QString const& modeId);
    void setRotation(int outputId, int rotation);
    void addOutput(int outputId, const QString& name);
    void removeOutput(int outputId);

private Q_SLOTS:
    void delayedInit();

private:
    QByteArray edid(int outputId) const;

    QString mConfigFile;
    mutable Disman::ConfigPtr mConfig;

    std::unique_ptr<Disman::Filer_controller> m_filer_controller;
};

#endif
