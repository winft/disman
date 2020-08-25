/*************************************************************************************
 *  Copyright 2012 - 2014  Daniel Vrátil <dvratil@redhat.com>                        *
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
#include "configmonitor.h"
#include "abstractbackend.h"
#include "backendinterface.h"
#include "backendmanager_p.h"
#include "configserializer_p.h"
#include "disman_debug.h"
#include "getconfigoperation.h"
#include "output.h"

#include <QDBusPendingCallWatcher>

using namespace Disman;

class Q_DECL_HIDDEN ConfigMonitor::Private : public QObject
{
    Q_OBJECT

public:
    Private(ConfigMonitor* q);

    void updateConfigs();
    void onBackendReady(org::kwinft::disman::backend* backend);
    void backendConfigChanged(const QVariantMap& configMap);
    void configDestroyed(QObject* removedConfig);
    void getConfigFinished(ConfigOperation* op);
    void updateConfigs(const Disman::ConfigPtr& newConfig);
    void edidReady(QDBusPendingCallWatcher* watcher);

    QList<QWeakPointer<Disman::Config>> watchedConfigs;

    QPointer<org::kwinft::disman::backend> mBackend;
    bool mFirstBackend;

    QMap<Disman::ConfigPtr, QList<int>> mPendingEDIDRequests;

private:
    ConfigMonitor* q;
};

ConfigMonitor::Private::Private(ConfigMonitor* q)
    : QObject(q)
    , mFirstBackend(true)
    , q(q)
{
}

void ConfigMonitor::Private::onBackendReady(org::kwinft::disman::backend* backend)
{
    Q_ASSERT(BackendManager::instance()->method() == BackendManager::OutOfProcess);
    if (backend == mBackend) {
        return;
    }

    if (mBackend) {
        disconnect(mBackend.data(),
                   &org::kwinft::disman::backend::configChanged,
                   this,
                   &ConfigMonitor::Private::backendConfigChanged);
    }

    mBackend = QPointer<org::kwinft::disman::backend>(backend);
    // If we received a new backend interface, then it's very likely that it is
    // because the backend process has crashed - just to be sure we haven't missed
    // any change, request the current config now and update our watched configs
    //
    // Only request the config if this is not initial backend request, because it
    // can happen that if a change happened before now, or before we get the config,
    // the result will be invalid. This can happen when Disman KDED launches and
    // detects changes need to be done.
    if (!mFirstBackend && !watchedConfigs.isEmpty()) {
        connect(new GetConfigOperation(),
                &GetConfigOperation::finished,
                this,
                &Private::getConfigFinished);
    }
    mFirstBackend = false;

    connect(mBackend.data(),
            &org::kwinft::disman::backend::configChanged,
            this,
            &ConfigMonitor::Private::backendConfigChanged);
}

void ConfigMonitor::Private::getConfigFinished(ConfigOperation* op)
{
    Q_ASSERT(BackendManager::instance()->method() == BackendManager::OutOfProcess);
    if (op->hasError()) {
        qCWarning(DISMAN) << "Failed to retrieve current config: " << op->errorString();
        return;
    }

    const Disman::ConfigPtr config = qobject_cast<GetConfigOperation*>(op)->config();
    updateConfigs(config);
}

void ConfigMonitor::Private::backendConfigChanged(const QVariantMap& configMap)
{
    Q_ASSERT(BackendManager::instance()->method() == BackendManager::OutOfProcess);
    ConfigPtr newConfig = ConfigSerializer::deserializeConfig(configMap);
    if (!newConfig) {
        qCWarning(DISMAN) << "Failed to deserialize config from DBus change notification";
        return;
    }

    for (auto const& output : newConfig->outputs()) {
        if (!output->edid()) {
            QDBusPendingReply<QByteArray> reply = mBackend->getEdid(output->id());
            mPendingEDIDRequests[newConfig].append(output->id());
            QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply);
            watcher->setProperty("outputId", output->id());
            watcher->setProperty("config", QVariant::fromValue(newConfig));
            connect(watcher,
                    &QDBusPendingCallWatcher::finished,
                    this,
                    &ConfigMonitor::Private::edidReady);
        }
    }

    if (mPendingEDIDRequests.contains(newConfig)) {
        qCDebug(DISMAN) << "Requesting missing EDID for outputs" << mPendingEDIDRequests[newConfig];
    } else {
        updateConfigs(newConfig);
    }
}

void ConfigMonitor::Private::edidReady(QDBusPendingCallWatcher* watcher)
{
    Q_ASSERT(BackendManager::instance()->method() == BackendManager::OutOfProcess);

    const int outputId = watcher->property("outputId").toInt();
    qCDebug(DISMAN) << "Received EDID for output" << outputId;

    const ConfigPtr config = watcher->property("config").value<Disman::ConfigPtr>();
    Q_ASSERT(mPendingEDIDRequests.contains(config));
    Q_ASSERT(mPendingEDIDRequests[config].contains(outputId));

    watcher->deleteLater();

    mPendingEDIDRequests[config].removeOne(outputId);

    const QDBusPendingReply<QByteArray> reply = *watcher;
    if (reply.isError()) {
        qCWarning(DISMAN) << "Error when retrieving EDID: " << reply.error().message();
    } else {
        const QByteArray edid = reply.argumentAt<0>();
        if (!edid.isEmpty()) {
            OutputPtr output = config->output(outputId);
            output->setEdid(edid);
        }
    }

    if (mPendingEDIDRequests[config].isEmpty()) {
        mPendingEDIDRequests.remove(config);
        updateConfigs(config);
    }
}

void ConfigMonitor::Private::updateConfigs(const Disman::ConfigPtr& newConfig)
{
    QMutableListIterator<QWeakPointer<Config>> iter(watchedConfigs);
    while (iter.hasNext()) {
        Disman::ConfigPtr config = iter.next().toStrongRef();
        if (!config) {
            iter.remove();
            continue;
        }

        config->apply(newConfig);
        iter.setValue(config.toWeakRef());
    }

    Q_EMIT q->configurationChanged();
}

void ConfigMonitor::Private::configDestroyed(QObject* removedConfig)
{
    for (auto iter = watchedConfigs.begin(); iter != watchedConfigs.end(); ++iter) {
        if (iter->toStrongRef() == removedConfig) {
            iter = watchedConfigs.erase(iter);
            // Iterate over the entire list in case there are duplicates
        }
    }
}

ConfigMonitor* ConfigMonitor::instance()
{
    static ConfigMonitor* s_instance = nullptr;

    if (s_instance == nullptr) {
        s_instance = new ConfigMonitor();
    }

    return s_instance;
}

ConfigMonitor::ConfigMonitor()
    : QObject()
    , d(new Private(this))
{
    if (BackendManager::instance()->method() == BackendManager::OutOfProcess) {
        connect(BackendManager::instance(),
                &BackendManager::backendReady,
                d,
                &ConfigMonitor::Private::onBackendReady);
        BackendManager::instance()->requestBackend();
    }
}

ConfigMonitor::~ConfigMonitor()
{
    delete d;
}

void ConfigMonitor::addConfig(const ConfigPtr& config)
{
    const QWeakPointer<Config> weakConfig = config.toWeakRef();
    if (!d->watchedConfigs.contains(weakConfig)) {
        connect(weakConfig.toStrongRef().data(), &QObject::destroyed, d, &Private::configDestroyed);
        d->watchedConfigs << weakConfig;
    }
}

void ConfigMonitor::removeConfig(const ConfigPtr& config)
{
    const QWeakPointer<Config> weakConfig = config.toWeakRef();
    if (d->watchedConfigs.contains(config)) {
        disconnect(
            weakConfig.toStrongRef().data(), &QObject::destroyed, d, &Private::configDestroyed);
        d->watchedConfigs.removeAll(config);
    }
}

void ConfigMonitor::connectInProcessBackend(Disman::AbstractBackend* backend)
{
    Q_ASSERT(BackendManager::instance()->method() == BackendManager::InProcess);
    connect(backend, &AbstractBackend::configChanged, [=](Disman::ConfigPtr config) {
        if (config.isNull()) {
            return;
        }
        qCDebug(DISMAN) << "Backend change!" << config;
        BackendManager::instance()->setConfig(config);
        d->updateConfigs(config);
    });
}

#include "configmonitor.moc"
