/*
 * Copyright (C) 2014  Daniel Vratil <dvratil@redhat.com>
 * Copyright 2015 Sebastian Kügler <sebas@kde.org>
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
/**
 * WARNING: This header is *not* part of public API and is subject to change.
 * There are not guarantees or API or ABI stability or compatibility between
 * releases
 */

#ifndef DISMAN_BACKENDMANAGER_H
#define DISMAN_BACKENDMANAGER_H

#include <QDBusServiceWatcher>
#include <QEventLoop>
#include <QFileInfoList>
#include <QObject>
#include <QPluginLoader>
#include <QProcess>
#include <QTimer>

#include <string>

#include "disman_export.h"
#include "types.h"

class QDBusPendingCallWatcher;
class OrgKwinftDismanBackendInterface;

namespace Disman
{

class Backend;

class DISMAN_EXPORT BackendManager : public QObject
{
    Q_OBJECT

public:
    enum Method { InProcess, OutOfProcess };

    static BackendManager* instance();
    ~BackendManager() override;

    Disman::ConfigPtr config() const;
    void set_config(Disman::ConfigPtr c);

    /** Choose which backend to use
     *
     * This method uses a couple of heuristics to pick the backend to be loaded:
     * - If the @p pre_select argument is specified and not empty it's used to filter the
     *   available backend list
     * - If specified, the DISMAN_BACKEND env var is considered (case insensitive)
     * - Otherwise, the wayland backend is picked when the runtime platform is Wayland
     *   (we assume kwin in this case
     * - Otherwise, if the runtime platform is X11, the XRandR backend is picked
     * - If neither is the case, we fall back to the QScreen backend, since that is the
     *   most generally applicable and may work on platforms not explicitly supported
     *
     * @return the backend plugin to load
     * @since 5.7
     */
    static QFileInfo preferred_backend(std::string const& pre_select = "");

    /** List installed backends
     * @return a list of installed backend plugins
     * @since 5.7
     */
    static QFileInfoList list_backends();

    /** Encapsulates the plugin loading logic.
     *
     * @param loader a pointer to the QPluginLoader, the caller is
     * responsible for its memory management.
     * @param name name of the backend plugin
     * @param arguments arguments, used for unit tests
     * @return a pointer to the backend loaded from the plugin
     * @since 5.6
     */
    static Disman::Backend*
    load_backend_plugin(QPluginLoader* loader, const QString& name, const QVariantMap& arguments);

    Disman::Backend* load_backend_in_process(const QString& name);

    BackendManager::Method method() const;
    void set_method(BackendManager::Method m);

    // For out-of-process operation
    void request_backend();
    void shutdown_backend();

Q_SIGNALS:
    void backend_ready(OrgKwinftDismanBackendInterface* backend);

private:
    friend class SetInProcessOperation;
    friend class InProcessConfigOperationPrivate;
    friend class SetConfigOperation;
    friend class SetConfigOperationPrivate;

    explicit BackendManager();
    static BackendManager* sInstance;

    void init_method();
    Q_INVOKABLE void emit_backend_ready();

    void start_backend(const QString& backend = QString(),
                       const QVariantMap& arguments = QVariantMap());
    void on_backend_request_done(QDBusPendingCallWatcher* watcher);
    void backend_service_unregistered(const QString& service_name);

    // For out-of-process operation
    void invalidate_interface();

    static const int sMaxCrashCount;
    OrgKwinftDismanBackendInterface* mInterface;
    int mCrashCount;

    QString mBackendService;
    QDBusServiceWatcher mServiceWatcher;
    Disman::ConfigPtr mConfig;
    QTimer mResetCrashCountTimer;
    bool mShuttingDown;
    int mRequestsCounter;
    QEventLoop mShutdownLoop;

    // For in-process operation
    QPluginLoader* mLoader;
    QPair<Disman::Backend*, QVariantMap> m_inProcessBackend;

    Method mMethod;
};

}

#endif
