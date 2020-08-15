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
#include "backendmanager_p.h"

#include "abstractbackend.h"
#include "backendinterface.h"
#include "config.h"
#include "configmonitor.h"
#include "configserializer_p.h"
#include "disman_debug.h"
#include "getconfigoperation.h"
#include "log.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QThread>
#include <QX11Info>

#include <memory>

using namespace Disman;

Q_DECLARE_METATYPE(org::kwinft::disman::backend*)

const int BackendManager::sMaxCrashCount = 4;

BackendManager* BackendManager::sInstance = nullptr;

BackendManager* BackendManager::instance()
{
    if (!sInstance) {
        sInstance = new BackendManager();
    }

    return sInstance;
}

BackendManager::BackendManager()
    : mInterface(nullptr)
    , mCrashCount(0)
    , mShuttingDown(false)
    , mRequestsCounter(0)
    , mLoader(nullptr)
    , mMethod(OutOfProcess)
{
    Log::instance();
    // Decide whether to run in, or out-of-process

    // if DISMAN_BACKEND_INPROCESS is set explicitly, we respect that
    const auto _inprocess = qgetenv("DISMAN_BACKEND_INPROCESS");
    if (!_inprocess.isEmpty()) {

        const QByteArrayList falses({QByteArray("0"), QByteArray("false")});
        if (!falses.contains(_inprocess.toLower())) {
            mMethod = InProcess;
        } else {
            mMethod = OutOfProcess;
        }
    } else {
        mMethod = OutOfProcess;
    }

    // TODO(romangg): fallback to in-process when dbus not available?

    initMethod();
}

void BackendManager::initMethod()
{
    if (mMethod == OutOfProcess) {
        qRegisterMetaType<org::kwinft::disman::backend*>("OrgKwinftDismanBackendInterface");

        mServiceWatcher.setConnection(QDBusConnection::sessionBus());
        connect(&mServiceWatcher,
                &QDBusServiceWatcher::serviceUnregistered,
                this,
                &BackendManager::backendServiceUnregistered);

        mResetCrashCountTimer.setSingleShot(true);
        mResetCrashCountTimer.setInterval(60000);
        connect(&mResetCrashCountTimer, &QTimer::timeout, this, [=]() { mCrashCount = 0; });
    }
}

void BackendManager::setMethod(BackendManager::Method m)
{
    if (mMethod == m) {
        return;
    }
    shutdownBackend();
    mMethod = m;
    initMethod();
}

BackendManager::Method BackendManager::method() const
{
    return mMethod;
}

BackendManager::~BackendManager()
{
    if (mMethod == InProcess) {
        shutdownBackend();
    }
}

QFileInfo BackendManager::preferred_backend(std::string const& pre_select)
{
    /**
     * Logic to pick a backend. It does this in order of priority:
     *   - pre_select argument is used if not empty,
     *   - env var DISMAN_BACKEND is considered,
     *   - if platform is X11, the XRandR backend is picked,
     *   - if platform is wayland, Wayland backend is picked,
     *   - if neither is the case, QScreen backend is picked,
     *   - the QScreen backend is also used as fallback.
     */

    auto env_select = qgetenv("DISMAN_BACKEND").toStdString();

    auto get_selection = [&]() -> std::string {
        if (pre_select.size()) {
            return pre_select;
        }
        if (env_select.size()) {
            return env_select;
        }

        if (QX11Info::isPlatformX11()) {
            return "randr";
        }
        if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"))) {
            return "wayland";
        }
        return "qscreen";
    };
    auto const select = get_selection();

    QFileInfo fallback;
    for (auto const& file_info : listBackends()) {
        // Here's the part where we do the match case-insensitive
        if (file_info.baseName().toLower()
            == QStringLiteral("%1").arg(QString::fromStdString(select).toLower())) {
            return file_info;
        }
        if (file_info.baseName() == QLatin1String("qscreen")) {
            fallback = file_info;
        }
    }
    qCWarning(DISMAN) << "No preferred backend found. Env var DISMAN_BACKEND was"
                      << (env_select.size() ? (std::string("set to:") + env_select).c_str()
                                            : "not set.")
                      << "Falling back to:" << fallback.fileName();
    return fallback;
}

QFileInfoList BackendManager::listBackends()
{
    // Compile a list of installed backends first
    const QStringList paths = QCoreApplication::libraryPaths();

    QFileInfoList finfos;
    for (const QString& path : paths) {
        const QDir dir(path + QLatin1String("/disman/"),
                       QString(),
                       QDir::SortFlags(QDir::QDir::Name),
                       QDir::NoDotAndDotDot | QDir::Files);
        finfos.append(dir.entryInfoList());
    }
    return finfos;
}

Disman::AbstractBackend* BackendManager::loadBackendPlugin(QPluginLoader* loader,
                                                           const QString& name,
                                                           const QVariantMap& arguments)
{
    const auto finfo = preferred_backend(name.toStdString());
    loader->setFileName(finfo.filePath());
    QObject* instance = loader->instance();
    if (!instance) {
        qCDebug(DISMAN) << loader->errorString();
        return nullptr;
    }

    auto backend = qobject_cast<Disman::AbstractBackend*>(instance);
    if (backend) {
        backend->init(arguments);
        if (!backend->isValid()) {
            qCDebug(DISMAN) << "Skipping" << backend->name() << "backend";
            delete backend;
            return nullptr;
        }
        // qCDebug(DISMAN) << "Loaded" << backend->name() << "backend";
        return backend;
    } else {
        qCDebug(DISMAN) << finfo.fileName() << "does not provide valid Disman backend";
    }

    return nullptr;
}

Disman::AbstractBackend* BackendManager::loadBackendInProcess(const QString& name)
{
    Q_ASSERT(mMethod == InProcess);
    if (mMethod == OutOfProcess) {
        qCWarning(DISMAN)
            << "You are trying to load a backend in process, while the BackendManager is set to "
               "use OutOfProcess communication. Use loadBackendPlugin() instead.";
        return nullptr;
    }
    if (m_inProcessBackend.first != nullptr
        && (name.isEmpty() || m_inProcessBackend.first->name() == name)) {
        return m_inProcessBackend.first;
    } else if (m_inProcessBackend.first != nullptr && m_inProcessBackend.first->name() != name) {
        shutdownBackend();
    }

    if (mLoader == nullptr) {
        mLoader = new QPluginLoader(this);
    }
    auto test_data_equals = QStringLiteral("TEST_DATA=");
    QVariantMap arguments;
    auto beargs = QString::fromLocal8Bit(qgetenv("DISMAN_BACKEND_ARGS"));
    if (beargs.startsWith(test_data_equals)) {
        arguments[QStringLiteral("TEST_DATA")] = beargs.remove(test_data_equals);
    }
    auto backend = BackendManager::loadBackendPlugin(mLoader, name, arguments);
    if (!backend) {
        return nullptr;
    }
    // qCDebug(DISMAN) << "Connecting ConfigMonitor to backend.";
    ConfigMonitor::instance()->connectInProcessBackend(backend);
    m_inProcessBackend = qMakePair<Disman::AbstractBackend*, QVariantMap>(backend, arguments);
    setConfig(backend->config());
    return backend;
}

void BackendManager::requestBackend()
{
    Q_ASSERT(mMethod == OutOfProcess);
    if (mInterface && mInterface->isValid()) {
        ++mRequestsCounter;
        QMetaObject::invokeMethod(this, "emitBackendReady", Qt::QueuedConnection);
        return;
    }

    // Another request already pending
    if (mRequestsCounter > 0) {
        return;
    }
    ++mRequestsCounter;

    const QByteArray args = qgetenv("DISMAN_BACKEND_ARGS");
    QVariantMap arguments;
    if (!args.isEmpty()) {
        QList<QByteArray> arglist = args.split(';');
        Q_FOREACH (const QByteArray& arg, arglist) {
            const int pos = arg.indexOf('=');
            if (pos == -1) {
                continue;
            }
            arguments.insert(QString::fromUtf8(arg.left(pos)), arg.mid(pos + 1));
        }
    }

    startBackend(QString::fromLatin1(qgetenv("DISMAN_BACKEND")), arguments);
}

void BackendManager::emitBackendReady()
{
    Q_ASSERT(mMethod == OutOfProcess);
    Q_EMIT backendReady(mInterface);
    --mRequestsCounter;
    if (mShutdownLoop.isRunning()) {
        mShutdownLoop.quit();
    }
}

void BackendManager::startBackend(const QString& backend, const QVariantMap& arguments)
{
    // This will autostart the launcher if it's not running already, calling
    // requestBackend(backend) will:
    //   a) if the launcher is started it will force it to load the correct backend,
    //   b) if the launcher is already running it will make sure it's running with
    //      the same backend as the one we requested and send an error otherwise
    QDBusConnection conn = QDBusConnection::sessionBus();
    QDBusMessage call = QDBusMessage::createMethodCall(QStringLiteral("org.kwinft.disman"),
                                                       QStringLiteral("/"),
                                                       QStringLiteral("org.kwinft.disman"),
                                                       QStringLiteral("requestBackend"));
    call.setArguments({backend, arguments});
    QDBusPendingCall pending = conn.asyncCall(call);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(pending);
    connect(
        watcher, &QDBusPendingCallWatcher::finished, this, &BackendManager::onBackendRequestDone);
}

void BackendManager::onBackendRequestDone(QDBusPendingCallWatcher* watcher)
{
    Q_ASSERT(mMethod == OutOfProcess);
    watcher->deleteLater();
    QDBusPendingReply<bool> reply = *watcher;
    // Most probably we requested an explicit backend that is different than the
    // one already loaded in the launcher
    if (reply.isError()) {
        qCWarning(DISMAN) << "Failed to request backend:" << reply.error().name() << ":"
                          << reply.error().message();
        invalidateInterface();
        emitBackendReady();
        return;
    }

    // Most probably request and explicit backend which is not available or failed
    // to initialize, or the launcher did not find any suitable backend for the
    // current platform.
    if (!reply.value()) {
        qCWarning(DISMAN) << "Failed to request backend: unknown error";
        invalidateInterface();
        emitBackendReady();
        return;
    }

    // The launcher has successfully loaded the backend we wanted and registered
    // it to DBus (hopefuly), let's try to get an interface for the backend.
    if (mInterface) {
        invalidateInterface();
    }
    mInterface = new org::kwinft::disman::backend(QStringLiteral("org.kwinft.disman"),
                                                  QStringLiteral("/backend"),
                                                  QDBusConnection::sessionBus());
    if (!mInterface->isValid()) {
        qCWarning(DISMAN) << "Backend successfully requested, but we failed to obtain a valid DBus "
                             "interface for it";
        invalidateInterface();
        emitBackendReady();
        return;
    }

    // The backend is GO, so let's watch for it's possible disappearance, so we
    // can invalidate the interface
    mServiceWatcher.addWatchedService(mBackendService);

    // Immediatelly request config
    connect(new GetConfigOperation(GetConfigOperation::NoEDID),
            &GetConfigOperation::finished,
            [&](ConfigOperation* op) {
                mConfig = qobject_cast<GetConfigOperation*>(op)->config();
                emitBackendReady();
            });
    // And listen for its change.
    connect(mInterface,
            &org::kwinft::disman::backend::configChanged,
            [&](const QVariantMap& newConfig) {
                mConfig = Disman::ConfigSerializer::deserializeConfig(newConfig);
            });
}

void BackendManager::backendServiceUnregistered(const QString& serviceName)
{
    Q_ASSERT(mMethod == OutOfProcess);
    mServiceWatcher.removeWatchedService(serviceName);

    invalidateInterface();
    requestBackend();
}

void BackendManager::invalidateInterface()
{
    Q_ASSERT(mMethod == OutOfProcess);
    delete mInterface;
    mInterface = nullptr;
    mBackendService.clear();
}

ConfigPtr BackendManager::config() const
{
    return mConfig;
}

void BackendManager::setConfig(ConfigPtr c)
{
    // qCDebug(DISMAN) << "BackendManager::setConfig, outputs:" << c->outputs().count();
    mConfig = c;
}

void BackendManager::shutdownBackend()
{
    if (mMethod == InProcess) {
        delete mLoader;
        mLoader = nullptr;
        m_inProcessBackend.second.clear();
        delete m_inProcessBackend.first;
        m_inProcessBackend.first = nullptr;
    } else {

        if (mBackendService.isEmpty() && !mInterface) {
            return;
        }

        // If there are some currently pending requests, then wait for them to
        // finish before quitting
        while (mRequestsCounter > 0) {
            mShutdownLoop.exec();
        }

        mServiceWatcher.removeWatchedService(mBackendService);
        mShuttingDown = true;

        QDBusMessage call = QDBusMessage::createMethodCall(QStringLiteral("org.kwinft.disman"),
                                                           QStringLiteral("/"),
                                                           QStringLiteral("org.kwinft.disman"),
                                                           QStringLiteral("quit"));
        // Call synchronously
        QDBusConnection::sessionBus().call(call);
        invalidateInterface();

        while (QDBusConnection::sessionBus().interface()->isServiceRegistered(
            QStringLiteral("org.kwinft.disman"))) {
            QThread::msleep(100);
        }
    }
}
