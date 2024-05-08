#pragma once

#include ".proto_stubs/requests.pb.h"
#include ".proto_stubs/responses.pb.h"
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

public:
    Room (QObject* parent = nullptr);
    bool start (QString& error_message);

public slots:
    void receiveRequestHandlerRoom (const RTS::Request& request_oneof, QSharedPointer<Session> session, quint64 request_id);

signals:
    void sendResponseRoom (const RTS::Response& response, const QSharedPointer<Session>& session, quint64 request_id);
    void receiveRequest (const RTS::Request& request, const QSharedPointer<Session>& session, quint64 request_id);
    void statsUpdated (quint32 player_count, quint32 ready_player_count, quint32 spectator_count);

private:
    bool fillUnit (quint32 id, const Unit& unit, RTS::Unit& m_unit);
    bool fillCorpse (quint32 id, const Corpse& corpse, RTS::Corpse& m_corpse);
    bool fillMissile (quint32 id, const Missile& missile, RTS::Missile& m_missile);
    void fillStopAction (const StopAction& stop_action, RTS::StopAction* m_stop_action);
    void fillMoveAction (const MoveAction& move_action, RTS::MoveAction* m_move_action);
    void fillAttackAction (const AttackAction& attack_action, RTS::AttackAction* m_attack_action);
    bool fillCastAction (const CastAction& cast_action, RTS::CastAction* m_cast_action);
    bool fillPerformingAttackAction (const PerformingAttackAction& performing_attack_action, RTS::PerformingAttackAction* m_performing_attack_action);
    bool fillPerformingCastAction (const PerformingCastAction& performing_cast_action, RTS::PerformingCastAction* m_performing_cast_action);

private slots:
    void readyHandler ();
    void tick ();

private:
    QVector<QSharedPointer<Session>> players;
    QSharedPointer<Session> red_team;
    QSharedPointer<Session> blue_team;
    QSharedPointer<QTimer> timer;
    QSharedPointer<MatchState> match_state;
    QMap<quint32, quint32> red_unit_id_client_to_server_map;
    QMap<quint32, quint32> blue_unit_id_client_to_server_map;
    QVector<QSharedPointer<Session>> spectators; // not gonna use for now
    // QVector<std::optional<Session*> > teams;
    void setError (RTS::Error* error, const std::string& error_message, RTS::ErrorCode error_code);
    void init_matchstate ();
    void emitStatsUpdated ();
    int sampling = 0;
};
