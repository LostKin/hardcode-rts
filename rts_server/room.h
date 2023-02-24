#pragma once

#include ".proto_stubs/session_level.pb.h"
#include "application.h"

#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QQueue>
#include <QMutex>
#include <QSharedPointer>


class Room: public QObject
{
    Q_OBJECT

private:
    QSharedPointer<Session> red_team;
    bool red_ready = false;
    QSharedPointer<Session> blue_team;
    bool blue_ready = false;
    QVector<QSharedPointer<Session> > spectators; // not gonna use for now
    //QVector<std::optional<Session*> > teams;
    void setError (RTS::Error* error, const std::string& error_message, RTS::ErrorCode error_code);

public:
    Room (QObject *parent = nullptr);
    bool start (QString& error_message);

signals:
    void sendResponseRoom(RTS::Response response, QSharedPointer<Session> session);
    void receiveRequest(RTS::Request request, QSharedPointer<Session> session);

private slots:
    void readyHandler();

public slots:
    void receiveRequestHandlerRoom (RTS::Request request_oneof, QSharedPointer<Session> session);
};
