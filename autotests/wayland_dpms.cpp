/*************************************************************************************
 *  Copyright 2016 by Sebastian Kügler <sebas@kde.org>                               *
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
#include <QCoreApplication>
#include <QObject>
#include <QSignalSpy>
#include <QtTest>

#include <Wrapland/Client/connection_thread.h>
#include <Wrapland/Client/dpms.h>
#include <Wrapland/Client/registry.h>

#include "server.h"

static const QString s_socketName = QStringLiteral("disman-test-wayland-backend-0");

Q_LOGGING_CATEGORY(DISMAN, "disman")

using namespace Wrapland::Client;

class TestDpmsClient : public QObject
{
    Q_OBJECT

public:
    explicit TestDpmsClient(QObject* parent = nullptr);

Q_SIGNALS:
    void dpmsAnnounced();

private Q_SLOTS:

    void initTestCase();
    void cleanupTestCase();
    void testDpmsConnect();

private:
    ConnectionThread* m_connection;
    QThread* m_thread;
    Registry* m_registry;

    Disman::WaylandTestServer* m_server;
};

TestDpmsClient::TestDpmsClient(QObject* parent)
    : QObject(parent)
    , m_server(nullptr)
{
    setenv("WAYLAND_DISPLAY", s_socketName.toLocal8Bit().constData(), true);
    m_server = new Disman::WaylandTestServer(this);
    m_server->start();
}

void TestDpmsClient::initTestCase()
{
    // setup connection
    m_connection = new Wrapland::Client::ConnectionThread;
    m_connection->setSocketName(s_socketName);
    QSignalSpy connectedSpy(m_connection, &Wrapland::Client::ConnectionThread::establishedChanged);
    QVERIFY(connectedSpy.isValid());
    m_connection->setSocketName(s_socketName);

    m_thread = new QThread(this);
    m_connection->moveToThread(m_thread);
    m_thread->start();

    m_connection->establishConnection();
    QVERIFY(connectedSpy.wait());

    QSignalSpy dpmsSpy(this, &TestDpmsClient::dpmsAnnounced);

    m_connection->establishConnection();
    QVERIFY(connectedSpy.wait(100));

    m_registry = new Wrapland::Client::Registry;
    m_registry->create(m_connection);
    QObject::connect(m_registry, &Registry::interfacesAnnounced, this, [this] {
        const bool hasDpms = m_registry->hasInterface(Registry::Interface::Dpms);
        if (hasDpms) {
            qDebug() << QStringLiteral("Compositor provides a DpmsManager");
        } else {
            qDebug() << QStringLiteral("Compositor does not provid a DpmsManager");
        }
        emit this->dpmsAnnounced();
    });
    m_registry->setup();

    QVERIFY(dpmsSpy.wait(100));
}

void TestDpmsClient::cleanupTestCase()
{
    m_thread->exit();
    m_thread->wait();
    delete m_registry;
    delete m_thread;
    delete m_connection;
}

void TestDpmsClient::testDpmsConnect()
{
    QVERIFY(m_registry->isValid());
}

QTEST_GUILESS_MAIN(TestDpmsClient)

#include "wayland_dpms.moc"
