/*************************************************************************
Copyright © 2020   Roman Gilg <subdiff@gmail.com>

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
#pragma once

#include <config.h>
#include <dismanwl_export.h>

#include <QEventLoop>
#include <QObject>
#include <QVector>

class QThread;

namespace Disman
{
class Output;
class WaylandOutput;
class WaylandScreen;

class DISMANWL_EXPORT WaylandInterface : public QObject
{
    Q_OBJECT

public:
    ~WaylandInterface() override;

    virtual void initConnection(QThread* thread) = 0;
    virtual bool isInitialized() const;

    // Compositor side names as keys
    virtual std::map<int, WaylandOutput*> outputMap() const = 0;

    virtual bool applyConfig(const Disman::ConfigPtr& newConfig) = 0;
    virtual void updateConfig(Disman::ConfigPtr& config) = 0;

Q_SIGNALS:
    void config_changed();
    void initialized();
    void connectionFailed(const QString& socketName);
    void outputsChanged();

protected:
    explicit WaylandInterface(QObject* parent = nullptr);

    virtual void insertOutput(WaylandOutput* output) = 0;
    virtual WaylandOutput* takeOutput(WaylandOutput* output) = 0;

    bool signalsBlocked() const;
    void blockSignals();
    void unblockSignals();

    void checkInitialized();

    void addOutput(WaylandOutput* output);
    virtual void handleDisconnect();

private:
    void removeOutput(WaylandOutput* output);

    /**
     * Finalize: when the output is is initialized, we put it in the known outputs map,
     * remove it from the list of initializing outputs, and emit config_changed().
     */
    virtual void initOutput(WaylandOutput* output);

    void tryPendingConfig();

    // Compositor side names
    QList<WaylandOutput*> m_initializingOutputs;
    int m_lastOutputId = -1;

    bool m_blockSignals;
    Disman::ConfigPtr m_dismanConfig;
    WaylandScreen* m_screen;
};

class DISMAN_EXPORT WaylandFactory : public QObject
{
    Q_OBJECT
public:
    WaylandFactory(QObject* parent = nullptr)
        : QObject(parent)
    {
    }
    ~WaylandFactory() override = default;

    virtual WaylandInterface* createInterface(QObject* parent = nullptr) = 0;
};

}

Q_DECLARE_INTERFACE(Disman::WaylandFactory, "org.kwinft.disman.waylandinterface")
