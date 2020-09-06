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
#include "setconfigoperation.h"

#include "abstractbackend.h"
#include "backendmanager_p.h"
#include "config.h"
#include "configoperation_p.h"
#include "configserializer_p.h"
#include "disman_debug.h"
#include "output.h"

#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>

using namespace Disman;

namespace Disman
{

class SetConfigOperationPrivate : public ConfigOperationPrivate
{
    Q_OBJECT

public:
    explicit SetConfigOperationPrivate(const Disman::ConfigPtr& config, ConfigOperation* qq);

    void backend_ready(org::kwinft::disman::backend* backend) override;
    void onConfigSet(QDBusPendingCallWatcher* watcher);
    void normalizeOutputPositions();

    Disman::ConfigPtr config;

private:
    Q_DECLARE_PUBLIC(SetConfigOperation)
};

}

SetConfigOperationPrivate::SetConfigOperationPrivate(const ConfigPtr& config, ConfigOperation* qq)
    : ConfigOperationPrivate(qq)
    , config(config)
{
}

void SetConfigOperationPrivate::backend_ready(org::kwinft::disman::backend* backend)
{
    ConfigOperationPrivate::backend_ready(backend);

    Q_Q(SetConfigOperation);

    if (!backend) {
        q->set_error(tr("Failed to prepare backend"));
        q->emit_result();
        return;
    }

    const QVariantMap map = ConfigSerializer::serialize_config(config).toVariantMap();
    if (map.isEmpty()) {
        q->set_error(tr("Failed to serialize request"));
        q->emit_result();
        return;
    }

    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(backend->setConfig(map), this);
    connect(
        watcher, &QDBusPendingCallWatcher::finished, this, &SetConfigOperationPrivate::onConfigSet);
}

void SetConfigOperationPrivate::onConfigSet(QDBusPendingCallWatcher* watcher)
{
    Q_Q(SetConfigOperation);

    QDBusPendingReply<QVariantMap> reply = *watcher;
    watcher->deleteLater();

    if (reply.isError()) {
        q->set_error(reply.error().message());
        q->emit_result();
        return;
    }

    config = ConfigSerializer::deserialize_config(reply.value());
    if (!config) {
        q->set_error(tr("Failed to deserialize backend response"));
    }

    q->emit_result();
}

SetConfigOperation::SetConfigOperation(const ConfigPtr& config, QObject* parent)
    : ConfigOperation(new SetConfigOperationPrivate(config, this), parent)
{
}

SetConfigOperation::~SetConfigOperation()
{
}

ConfigPtr SetConfigOperation::config() const
{
    Q_D(const SetConfigOperation);
    return d->config;
}

void SetConfigOperation::start()
{
    Q_D(SetConfigOperation);
    d->normalizeOutputPositions();
    if (BackendManager::instance()->method() == BackendManager::InProcess) {
        auto backend = d->loadBackend();
        backend->set_config(d->config);
        emit_result();
    } else {
        d->request_backend();
    }
}

void SetConfigOperationPrivate::normalizeOutputPositions()
{
    if (!config) {
        return;
    }
    double offsetX = INT_MAX;
    double offsetY = INT_MAX;
    Q_FOREACH (const Disman::OutputPtr& output, config->outputs()) {
        if (!output->positionable()) {
            continue;
        }
        offsetX = qMin(output->geometry().left(), offsetX);
        offsetY = qMin(output->geometry().top(), offsetY);
    }

    if (!offsetX && !offsetY) {
        return;
    }
    qCDebug(DISMAN) << "Correcting output positions by:" << QPoint(offsetX, offsetY);
    Q_FOREACH (const Disman::OutputPtr& output, config->outputs()) {
        if (!output->enabled()) {
            continue;
        }
        auto newPos
            = QPointF(output->geometry().left() - offsetX, output->geometry().top() - offsetY);
        qCDebug(DISMAN) << "Moved output from" << output->geometry().topLeft() << "to" << newPos;
        output->set_position(newPos);
    }
}

#include "setconfigoperation.moc"
