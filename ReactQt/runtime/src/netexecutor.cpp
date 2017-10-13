
/**
 * Copyright (C) 2016, Canonical Ltd.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 * Author: Justin McPherson <justin.mcpherson@canonical.com>
 *
 */

#include <QJsonDocument>
#include <QObject>
#include <QQueue>
#include <QStateMachine>
#include <QString>
#include <QTcpSocket>
#include <QUrl>

#include <QDebug>

#include "netexecutor.h"

namespace {
struct RegisterClass {
    RegisterClass() {
        qRegisterMetaType<NetExecutor*>();
    }
} registerClass;
}

class NetExecutorPrivate : public QObject {
    Q_OBJECT
public:
    QString serverHost = "localhost";
    QTcpSocket* socket = nullptr;
    QStateMachine* machina = nullptr;
    QByteArray inputBuffer;
    QQueue<QByteArray> requestQueue;
    QQueue<Executor::ExecuteCallback> responseQueue;

    void processRequests() {
        bool socketNotConnected = socket->state() != QAbstractSocket::ConnectedState;
        bool queueEmpty = requestQueue.isEmpty();
        if (socketNotConnected || queueEmpty) {
            qDebug() << "!socket Process Request ERROR!, not connected: " << socketNotConnected
                     << " queue empty: " << queueEmpty;
            return;
        }

        QByteArray request = requestQueue.dequeue();
        quint32 length = request.size();
        socket->write((const char*)&length, sizeof(length));
        socket->write(request.constData(), request.size());
    }

public Q_SLOTS:
    void readReply() {
        QByteArray data = socket->readAll();

        Executor::ExecuteCallback callback = responseQueue.dequeue();
        if (callback) {
            QJsonDocument doc;
            doc = QJsonDocument::fromJson(data);
            callback(doc);
        }
    }
};

NetExecutor::NetExecutor(QObject* parent) : Executor(parent), d_ptr(new NetExecutorPrivate) {
    Q_D(NetExecutor);
    QString serverHost = qgetenv("REACT_SERVER_HOST");
    if (!serverHost.isEmpty())
        d->serverHost = serverHost;

    d->socket = new QTcpSocket(this);
    connect(d->socket, SIGNAL(readyRead()), d, SLOT(readReply()));

    d->machina = new QStateMachine(this);

    QState* initialState = new QState();
    QState* errorState = new QState();
    QState* readyState = new QState();

    initialState->addTransition(d->socket, SIGNAL(connected()), readyState);
    readyState->addTransition(d->socket, SIGNAL(error(QAbstractSocket::SocketError)), errorState);
    readyState->addTransition(d->socket, SIGNAL(disconnected()), errorState);

    connect(initialState, &QAbstractState::entered, [=] { d->socket->connectToHost(d->serverHost, 5000); });
    connect(readyState, &QAbstractState::entered, [=] { d->processRequests(); });
    connect(errorState, &QAbstractState::entered, [=] { d->machina->stop(); });

    d->machina->addState(initialState);
    d->machina->addState(errorState);
    d->machina->addState(readyState);
    d->machina->setInitialState(initialState);
}

NetExecutor::~NetExecutor() {}

QString NetExecutor::serverHost() const {
    return d_func()->serverHost;
}

void NetExecutor::setServerHost(const QString& serverHost) {
    Q_D(NetExecutor);
    if (d->serverHost == serverHost)
        return;
    d->serverHost = serverHost;
}

void NetExecutor::init() {
    d_func()->machina->start();
}

void NetExecutor::injectJson(const QString& name, const QVariant& data) {
    QJsonDocument doc = QJsonDocument::fromVariant(data);
    processRequest(name.toLocal8Bit() + "=" + doc.toJson(QJsonDocument::Compact));
}

void NetExecutor::executeApplicationScript(const QByteArray& script, const QUrl& /*sourceUrl*/) {
    processRequest(script, [=](const QJsonDocument&) { Q_EMIT applicationScriptDone(); });
}

void NetExecutor::executeJSCall(const QString& method,
                                const QVariantList& args,
                                const Executor::ExecuteCallback& callback) {
    QByteArrayList stringifiedArgs;
    for (const QVariant& arg : args) {
        if (arg.type() == QVariant::List || arg.type() == QVariant::Map) {
            QJsonDocument doc = QJsonDocument::fromVariant(arg);
            stringifiedArgs += doc.toJson(QJsonDocument::Compact);
        } else {
            stringifiedArgs += '"' + arg.toString().toLocal8Bit() + '"';
        }
    }

    processRequest(QByteArray("__fbBatchedBridge.") + method.toLocal8Bit() + "(" + stringifiedArgs.join(',') + ");",
                   callback);
}

void NetExecutor::processRequest(const QByteArray& request, const Executor::ExecuteCallback& callback) {
    Q_D(NetExecutor);

    d->requestQueue.enqueue(request);
    d->responseQueue.enqueue(callback);
    d->processRequests();
}

#include "netexecutor.moc"
