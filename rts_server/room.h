#pragma once

#include ".proto_stubs/requests.pb.h"
#include ".proto_stubs/responses.pb.h"
#include "application.h"
#include "matchstate.h"

#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QMutex>
#include <QTimer>
#include <memory>


class Room: public QObject
{
    Q_OBJECT

public:
    Room (QObject* parent = nullptr);
    bool start (std::string& error_message);

public slots:
    void receiveRequestHandlerRoom (const RTS::Request& request_oneof, std::shared_ptr<Session> session, uint64_t request_id);

signals:
    void sendResponseRoom (const RTS::Response& response, const std::shared_ptr<Session>& session, uint64_t request_id);
    void receiveRequest (const RTS::Request& request, const std::shared_ptr<Session>& session, uint64_t request_id);
    void statsUpdated (uint32_t player_count, uint32_t ready_player_count, uint32_t spectator_count);

private slots:
    void readyHandler ();
    void tick ();

private:
    std::vector<std::shared_ptr<Session>> players;
    std::shared_ptr<Session> red_team;
    std::shared_ptr<Session> blue_team;
    std::shared_ptr<QTimer> timer;
    std::shared_ptr<MatchState> match_state;
    std::map<uint32_t, uint32_t> red_unit_id_client_to_server_map;
    std::map<uint32_t, uint32_t> blue_unit_id_client_to_server_map;
    std::vector<std::shared_ptr<Session>> spectators; // not gonna use for now
    void setError (RTS::Error* error, const std::string& error_message, RTS::ErrorCode error_code);
    void init_matchstate ();
    void emitStatsUpdated ();
    int sampling = 0;
};
