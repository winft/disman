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

#include "backend.h"
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

    // if DISMAN_IN_PROCESS is set explicitly, we respect that
    auto const in_process = qgetenv("DISMAN_IN_PROCESS");
    if (!in_process.isEmpty()) {

        const QByteArrayList falses({QByteArray("0"), QByteArray("false")});
        if (!falses.contains(in_process.toLower())) {
            mMethod = InProcess;
        } else {
            mMethod = OutOfProcess;
        }
    } else {
        mMethod = OutOfProcess;
    }

    // TODO(romangg): fallback to in-process when dbus not available?

    init_method();
}

void BackendManager::init_method()
{
    if (mMethod == OutOfProcess) {
        qRegisterMetaType<org::kwinft::disman::backend*>("OrgKwinftDismanBackendInterface");

        mServiceWatcher.setConnection(QDBusConnection::sessionBus());
        connect(&mServiceWatcher,
                &QDBusServiceWatcher::serviceUnregistered,
                this,
                &BackendManager::backend_service_unregistered);

        mResetCrashCountTimer.setSingleShot(true);
        mResetCrashCountTimer.setInterval(60000);
        connect(&mResetCrashCountTimer, &QTimer::timeout, this, [=]() { mCrashCount = 0; });
    }
}

void BackendManager::set_method(BackendManager::Method m)
{
    if (mMethod == m) {
        return;
    }
    shutdown_backend();
    mMethod = m;
    init_method();
}

BackendManager::Method BackendManager::method() const
{
    return mMethod;
}

BackendManager::~BackendManager()
{
    if (mMethod == InProcess) {
        shutdown_backend();
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

        // If XDG_SESSION_TYPE is defined and indicates a certain windowing system we prefer
        // that variable, since it likely reflects correctly the current session setup.
        auto const session_type = qgetenv("XDG_SESSION_TYPE");
        if (session_type == "wayland") {
            return "wayland";
        }
        if (session_type == "x11") {
            return "randr";
        }

        if (auto display = qgetenv("WAYLAND_DISPLAY"); !display.isEmpty()) {
            auto dsp_str = QString::fromLatin1(display);

            auto socket_exists = [&dsp_str] {
                if (QDir::isAbsolutePath(dsp_str)) {
                    return QFile(dsp_str).exists();
                }
                auto const locations
                    = QStandardPaths::standardLocations(QStandardPaths::RuntimeLocation);
                for (auto const& dir : qAsConst(locations)) {
                    if (QFileInfo(QDir(dir), dsp_str).exists()) {
                        return true;
                    }
                }
                return false;
            };

            if (socket_exists()) {
                return "wayland";
            }
        }
        if (!qEnvironmentVariableIsEmpty("DISPLAY")) {
            return "randr";
        }
        return "qscreen";
    };
    auto const select = get_selection();
    qCDebug(DISMAN) << "Selection for preferred backend:" << select.c_str();

    QFileInfo fallback;
    auto const& backends = list_backends();
    for (auto const& file_info : qAsConst(backends)) {
        if (file_info.baseName().toStdString() == select) {
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

QFileInfoList BackendManager::list_backends()
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

Disman::Backend* BackendManager::load_backend_plugin(QPluginLoader* loader,
                                                     const QString& name,
                                                     const QVariantMap& arguments)
{
    const auto finfo = preferred_backend(name.toStdString());
    loader->setFileName(finfo.filePath());
    qCDebug(DISMAN) << "Loading backend plugin:" << finfo.filePath();

    QObject* instance = loader->instance();
    if (!instance) {
        qCDebug(DISMAN) << loader->errorString();
        return nullptr;
    }

    auto backend = qobject_cast<Disman::Backend*>(instance);
    if (backend) {
        backend->init(arguments);
        if (!backend->valid()) {
            qCDebug(DISMAN) << "Skipping" << backend->name() << "backend";
            delete backend;
            return nullptr;
        }
        qCDebug(DISMAN) << "Loaded successfully backend:" << backend->name();
        return backend;
    } else {
        qCWarning(DISMAN) << finfo.fileName() << "does not provide a valid Disman backend.";
    }

    return nullptr;
}

Disman::Backend* BackendManager::load_backend_in_process(const QString& name)
{
    Q_ASSERT(mMethod == InProcess);
    if (mMethod == OutOfProcess) {
        qCWarning(DISMAN)
            << "You are trying to load a backend in process, while the BackendManager is set to "
               "use OutOfProcess communication. Use load_backend_plugin() instead.";
        return nullptr;
    }
    if (m_inProcessBackend.first != nullptr
        && (name.isEmpty() || m_inProcessBackend.first->name() == name)) {
        return m_inProcessBackend.first;
    } else if (m_inProcessBackend.first != nullptr && m_inProcessBackend.first->name() != name) {
        shutdown_backend();
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
    auto backend = BackendManager::load_backend_plugin(mLoader, name, arguments);
    if (!backend) {
        return nullptr;
    }
    // qCDebug(DISMAN) << "Connecting ConfigMonitor to backend.";
    ConfigMonitor::instance()->connect_in_process_backend(backend);
    m_inProcessBackend = {backend, arguments};
    set_config(backend->config());
    return backend;
}

void BackendManager::request_backend()
{
    Q_ASSERT(mMethod == OutOfProcess);
    if (mInterface && mInterface->isValid()) {
        ++mRequestsCounter;
        QMetaObject::invokeMethod(this, "emit_backend_ready", Qt::QueuedConnection);
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

    start_backend(QString::fromLatin1(qgetenv("DISMAN_BACKEND")), arguments);
}

void BackendManager::emit_backend_ready()
{
    Q_ASSERT(mMethod == OutOfProcess);
    Q_EMIT backend_ready(mInterface);
    --mRequestsCounter;
    if (mShutdownLoop.isRunning()) {
        mShutdownLoop.quit();
    }
}

void BackendManager::start_backend(const QString& backend, const QVariantMap& arguments)
{
    // This will autostart the launcher if it's not running already, calling
    // request_backend(backend) will:
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
    connect(watcher,
            &QDBusPendingCallWatcher::finished,
            this,
            &BackendManager::on_backend_request_done);
}

void BackendManager::on_backend_request_done(QDBusPendingCallWatcher* watcher)
{
    Q_ASSERT(mMethod == OutOfProcess);
    watcher->deleteLater();
    QDBusPendingReply<bool> reply = *watcher;
    // Most probably we requested an explicit backend that is different than the
    // one already loaded in the launcher
    if (reply.isError()) {
        qCWarning(DISMAN) << "Failed to request backend:" << reply.error().name() << ":"
                          << reply.error().message();
        invalidate_interface();
        emit_backend_ready();
        return;
    }

    // Most probably request and explicit backend which is not available or failed
    // to initialize, or the launcher did not find any suitable backend for the
    // current platform.
    if (!reply.value()) {
        qCWarning(DISMAN) << "Failed to request backend: unknown error";
        invalidate_interface();
        emit_backend_ready();
        return;
    }

    // The launcher has successfully loaded the backend we wanted and registered
    // it to DBus (hopefuly), let's try to get an interface for the backend.
    if (mInterface) {
        invalidate_interface();
    }
    mInterface = new org::kwinft::disman::backend(QStringLiteral("org.kwinft.disman"),
                                                  QStringLiteral("/backend"),
                                                  QDBusConnection::sessionBus());
    if (!mInterface->isValid()) {
        qCWarning(DISMAN) << "Backend successfully requested, but we failed to obtain a valid DBus "
                             "interface for it";
        invalidate_interface();
        emit_backend_ready();
        return;
    }

    // The backend is GO, so let's watch for it's possible disappearance, so we
    // can invalidate the interface
    mServiceWatcher.addWatchedService(mBackendService);

    // Immediatelly request config
    connect(new GetConfigOperation, &GetConfigOperation::finished, this, [&](ConfigOperation* op) {
        mConfig = qobject_cast<GetConfigOperation*>(op)->config();
        emit_backend_ready();
    });
    // And listen for its change.
    connect(mInterface,
            &org::kwinft::disman::backend::configChanged,
            this,
            [&](const QVariantMap& newConfig) {
                mConfig = Disman::ConfigSerializer::deserialize_config(newConfig);
            });
}

void BackendManager::backend_service_unregistered(const QString& service_name)
{
    Q_ASSERT(mMethod == OutOfProcess);
    mServiceWatcher.removeWatchedService(service_name);

    invalidate_interface();
    request_backend();
}

void BackendManager::invalidate_interface()
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

void BackendManager::set_config(ConfigPtr c)
{
    mConfig = c;
}

void BackendManager::shutdown_backend()
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
        invalidate_interface();

        while (QDBusConnection::sessionBus().interface()->isServiceRegistered(
            QStringLiteral("org.kwinft.disman"))) {
            QThread::msleep(100);
        }
    }
}
