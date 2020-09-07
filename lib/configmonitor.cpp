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

Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr)

using namespace Disman;

class Q_DECL_HIDDEN ConfigMonitor::Private : public QObject
{
    Q_OBJECT

public:
    Private(ConfigMonitor* q);

    void update_configs();
    void on_backend_ready(org::kwinft::disman::backend* backend);
    void backend_config_changed(const QVariantMap& configMap);
    void config_destroyed(QObject* removedConfig);
    void get_config_finished(ConfigOperation* op);
    void update_configs(const Disman::ConfigPtr& newConfig);
    bool has_config(ConfigPtr const& config) const;

    QList<std::weak_ptr<Disman::Config>> watched_configs;

    QPointer<org::kwinft::disman::backend> mBackend;
    bool mFirstBackend;

private:
    ConfigMonitor* q;
};

ConfigMonitor::Private::Private(ConfigMonitor* q)
    : QObject(q)
    , mFirstBackend(true)
    , q(q)
{
}

void ConfigMonitor::Private::on_backend_ready(org::kwinft::disman::backend* backend)
{
    Q_ASSERT(BackendManager::instance()->method() == BackendManager::OutOfProcess);
    if (backend == mBackend) {
        return;
    }

    if (mBackend) {
        disconnect(mBackend.data(),
                   &org::kwinft::disman::backend::configChanged,
                   this,
                   &ConfigMonitor::Private::backend_config_changed);
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
    if (!mFirstBackend && !watched_configs.isEmpty()) {
        connect(new GetConfigOperation(),
                &GetConfigOperation::finished,
                this,
                &Private::get_config_finished);
    }
    mFirstBackend = false;

    connect(mBackend.data(),
            &org::kwinft::disman::backend::configChanged,
            this,
            &ConfigMonitor::Private::backend_config_changed);
}

void ConfigMonitor::Private::get_config_finished(ConfigOperation* op)
{
    Q_ASSERT(BackendManager::instance()->method() == BackendManager::OutOfProcess);
    if (op->has_error()) {
        qCWarning(DISMAN) << "Failed to retrieve current config: " << op->error_string();
        return;
    }

    const Disman::ConfigPtr config = qobject_cast<GetConfigOperation*>(op)->config();
    update_configs(config);
}

void ConfigMonitor::Private::backend_config_changed(const QVariantMap& configMap)
{
    Q_ASSERT(BackendManager::instance()->method() == BackendManager::OutOfProcess);

    ConfigPtr newConfig = ConfigSerializer::deserialize_config(configMap);
    if (!newConfig) {
        qCWarning(DISMAN) << "Failed to deserialize config from DBus change notification";
        return;
    }
    update_configs(newConfig);
}

void ConfigMonitor::Private::update_configs(const Disman::ConfigPtr& newConfig)
{
    QMutableListIterator<std::weak_ptr<Config>> iter(watched_configs);
    while (iter.hasNext()) {
        Disman::ConfigPtr config = iter.next().lock();
        if (!config) {
            iter.remove();
            continue;
        }

        config->apply(newConfig);
        iter.setValue(config);
    }

    Q_EMIT q->configuration_changed();
}

void ConfigMonitor::Private::config_destroyed(QObject* removedConfig)
{
    for (auto iter = watched_configs.begin(); iter != watched_configs.end(); ++iter) {
        if (iter->lock().get() == removedConfig) {
            iter = watched_configs.erase(iter);
            // Iterate over the entire list in case there are duplicates
        }
    }
}

bool ConfigMonitor::Private::has_config(ConfigPtr const& config) const
{
    auto& cfgs = watched_configs;
    return std::find_if(cfgs.cbegin(),
                        cfgs.cend(),
                        [config](auto const& cfg) { return cfg.lock().get() == config.get(); })
        != cfgs.cend();
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
                &BackendManager::backend_ready,
                d,
                &ConfigMonitor::Private::on_backend_ready);
        BackendManager::instance()->request_backend();
    }
}

ConfigMonitor::~ConfigMonitor()
{
    delete d;
}

void ConfigMonitor::add_config(const ConfigPtr& config)
{
    if (d->has_config(config)) {
        return;
    }
    connect(config.get(), &QObject::destroyed, d, &Private::config_destroyed);
    d->watched_configs << config;
}

void ConfigMonitor::remove_config(const ConfigPtr& config)
{
    if (!d->has_config(config)) {
        return;
    }

    disconnect(config.get(), &QObject::destroyed, d, &Private::config_destroyed);
    auto& cfgs = d->watched_configs;
    cfgs.erase(
        std::remove_if(cfgs.begin(),
                       cfgs.end(),
                       [config](auto const& cfg) { return cfg.lock().get() == config.get(); }),
        cfgs.end());
}

void ConfigMonitor::connect_in_process_backend(Disman::AbstractBackend* backend)
{
    Q_ASSERT(BackendManager::instance()->method() == BackendManager::InProcess);
    connect(backend, &AbstractBackend::config_changed, [=](Disman::ConfigPtr config) {
        if (!config) {
            return;
        }
        qCDebug(DISMAN) << "Backend change!" << config;
        BackendManager::instance()->set_config(config);
        d->update_configs(config);
    });
}

#include "configmonitor.moc"
