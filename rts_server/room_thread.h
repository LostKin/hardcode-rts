#pragma once

#include "room.h"
#include "application.h"

#include ".proto_stubs/requests.pb.h"
#include ".proto_stubs/responses.pb.h"

#include <QThread>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QSharedPointer>

class RoomThread: public QThread
{
    Q_OBJECT

public:
    RoomThread (const QString& name, QObject* parent = nullptr);
    const QString& name () const;
    const QString& errorMessage () const;
    quint32 playerCount () const;
    quint32 readyPlayerCount () const;
    quint32 spectatorCount () const;

protected:
    void run () override;

private:
    const QString name_;
    int return_code = 0;
    QString error_message;
    QSharedPointer<Room> room;
    quint32 player_count = 0;
    quint32 ready_player_count = 0;
    quint32 spectator_count = 0;

signals:
    void receiveRequest (const RTS::Request& request_oneof, const QSharedPointer<Session>& session, quint64 request_id);
    void sendRequest (const RTS::Request& request_oneof, const QSharedPointer<Session>& session, quint64 request_id);
    void sendResponse (const RTS::Response& response, const QSharedPointer<Session>& session, quint64 request_id);

public slots:
    void receiveRequestHandler (const RTS::Request& request_oneof, const QSharedPointer<Session>& session, quint64 request_id);
    void sendResponseHandler (const RTS::Response& response, const QSharedPointer<Session>& session, quint64 request_id);

private slots:
    void updateStats (quint32 player_count, quint32 ready_player_count, quint32 spectator_count);
};
