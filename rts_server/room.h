#pragma once

#include ".proto_stubs/session_level.pb.h"
#include "application.h"
#include "matchstate.h"

#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QQueue>
#include <QMutex>
#include <QSharedPointer>
#include <QTimer>

class Room: public QObject
{
    Q_OBJECT

private:
    QVector<QSharedPointer<Session>> players;
    QSharedPointer<Session> red_team;
    QSharedPointer<Session> blue_team;
    QSharedPointer<QTimer> timer;
    QSharedPointer<MatchState> match_state;
    QMap<quint32, quint32> red_client_to_server;
    QMap<quint32, quint32> blue_client_to_server;
    QVector<QSharedPointer<Session>> spectators; // not gonna use for now
    // QVector<std::optional<Session*> > teams;
    void setError (RTS::Error* error, const std::string& error_message, RTS::ErrorCode error_code);
    void init_matchstate ();
    int sampling = 0;

public:
    Room (QObject* parent = nullptr);
    bool start (QString& error_message);

signals:
    void sendResponseRoom (RTS::Response response, QSharedPointer<Session> session);
    void receiveRequest (RTS::Request request, QSharedPointer<Session> session);

private slots:
    void readyHandler ();
    void tick ();

public slots:
    void receiveRequestHandlerRoom (RTS::Request request_oneof, QSharedPointer<Session> session);
};
