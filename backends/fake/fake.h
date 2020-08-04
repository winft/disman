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

#include "abstractbackend.h"

#include <QLoggingCategory>
#include <QObject>

class Fake : public Disman::AbstractBackend
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kf5.disman.backends.fake")

public:
    explicit Fake();
    ~Fake() override;

    void init(const QVariantMap& arguments) override;

    QString name() const override;
    QString serviceName() const override;
    Disman::ConfigPtr config() const override;
    void setConfig(const Disman::ConfigPtr& config) override;
    QByteArray edid(int outputId) const override;
    bool isValid() const override;

    void setEnabled(int outputId, bool enabled);
    void setPrimary(int outputId, bool primary);
    void setCurrentModeId(int outputId, const QString& modeId);
    void setRotation(int outputId, int rotation);
    void addOutput(int outputId, const QString& name);
    void removeOutput(int outputId);

private Q_SLOTS:
    void delayedInit();

private:
    QString mConfigFile;
    mutable Disman::ConfigPtr mConfig;
};

#endif
