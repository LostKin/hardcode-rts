#pragma once

#include "room.h"
#include "application.h"

#include ".proto_stubs/session_level.pb.h"

#include <QThread>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QSharedPointer>

class RoomThread: public QThread
{
    Q_OBJECT
public:
    RoomThread (QObject *parent = nullptr);
    const QString& errorMessage ();

protected:
    void run () override;

private:
    int return_code = 0;
    QString error_message;
    QSharedPointer<Room> room;

signals:
    void receiveRequest (RTS::Request request_oneof, QSharedPointer<Session> session);
    void sendRequest (RTS::Request request_oneof, QSharedPointer<Session> session);
    void sendResponse (RTS::Response response, QSharedPointer<Session> session);
    
public slots:
    void receiveRequestHandler (RTS::Request request_oneof, QSharedPointer<Session> session);
    void sendResponseHandler (RTS::Response response, QSharedPointer<Session> session);
};
