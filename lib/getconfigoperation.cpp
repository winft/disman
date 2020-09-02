/*
 * Copyright (C) 2014  Daniel Vratil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#include "getconfigoperation.h"
#include "backendinterface.h"
#include "backendmanager_p.h"
#include "config.h"
#include "configoperation_p.h"
#include "configserializer_p.h"
#include "log.h"
#include "output.h"

using namespace Disman;

namespace Disman
{

class GetConfigOperationPrivate : public ConfigOperationPrivate
{
    Q_OBJECT

public:
    GetConfigOperationPrivate(GetConfigOperation* qq);

    void backendReady(org::kwinft::disman::backend* backend) override;
    void onConfigReceived(QDBusPendingCallWatcher* watcher);

public:
    ConfigPtr config;

    // For out-of-process
    QPointer<org::kwinft::disman::backend> mBackend;

private:
    Q_DECLARE_PUBLIC(GetConfigOperation)
};

}

GetConfigOperationPrivate::GetConfigOperationPrivate(GetConfigOperation* qq)
    : ConfigOperationPrivate(qq)
{
}

void GetConfigOperationPrivate::backendReady(org::kwinft::disman::backend* backend)
{
    Q_ASSERT(BackendManager::instance()->method() == BackendManager::OutOfProcess);
    ConfigOperationPrivate::backendReady(backend);

    Q_Q(GetConfigOperation);

    if (!backend) {
        q->setError(tr("Failed to prepare backend"));
        q->emitResult();
        return;
    }

    mBackend = backend;
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(mBackend->getConfig(), this);
    connect(watcher,
            &QDBusPendingCallWatcher::finished,
            this,
            &GetConfigOperationPrivate::onConfigReceived);
}

void GetConfigOperationPrivate::onConfigReceived(QDBusPendingCallWatcher* watcher)
{
    Q_ASSERT(BackendManager::instance()->method() == BackendManager::OutOfProcess);
    Q_Q(GetConfigOperation);

    QDBusPendingReply<QVariantMap> reply = *watcher;
    watcher->deleteLater();
    if (reply.isError()) {
        q->setError(reply.error().message());
        q->emitResult();
        return;
    }

    config = ConfigSerializer::deserializeConfig(reply.value());
    if (!config) {
        q->setError(tr("Failed to deserialize backend response"));
    }

    q->emitResult();
}

GetConfigOperation::GetConfigOperation(QObject* parent)
    : ConfigOperation(new GetConfigOperationPrivate(this), parent)
{
}

GetConfigOperation::~GetConfigOperation()
{
}

Disman::ConfigPtr GetConfigOperation::config() const
{
    Q_D(const GetConfigOperation);
    return d->config;
}

void GetConfigOperation::start()
{
    Q_D(GetConfigOperation);
    if (BackendManager::instance()->method() == BackendManager::InProcess) {
        auto backend = d->loadBackend();
        if (!backend) {
            return; // loadBackend() already set error and called emitResult() for us
        }
        d->config = backend->config()->clone();
        emitResult();
    } else {
        d->requestBackend();
    }
}

#include "getconfigoperation.moc"
