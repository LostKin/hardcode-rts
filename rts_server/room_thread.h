#pragma once

#include "room.h"
#include "application.h"

#include "requests.pb.h"
#include "responses.pb.h"

#include <QThread>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <memory>

class RoomThread: public QThread
{
    Q_OBJECT

public:
    RoomThread (const std::string& name, QObject* parent = nullptr);
    const std::string& name () const;
    const std::string& errorMessage () const;
    uint32_t playerCount () const;
    uint32_t readyPlayerCount () const;
    uint32_t spectatorCount () const;

protected:
    void run () override;

private:
    const std::string name_;
    int return_code = 0;
    std::string error_message;
    std::shared_ptr<Room> room;
    uint32_t player_count = 0;
    uint32_t ready_player_count = 0;
    uint32_t spectator_count = 0;

signals:
    void receiveRequest (const RTS::Request& request_oneof, const std::shared_ptr<Session>& session, uint64_t request_id);
    void sendRequest (const RTS::Request& request_oneof, const std::shared_ptr<Session>& session, uint64_t request_id);
    void sendResponse (const RTS::Response& response, const std::shared_ptr<Session>& session, uint64_t request_id);

public slots:
    void receiveRequestHandler (const RTS::Request& request_oneof, const std::shared_ptr<Session>& session, uint64_t request_id);
    void sendResponseHandler (const RTS::Response& response, const std::shared_ptr<Session>& session, uint64_t request_id);

private slots:
    void updateStats (uint32_t player_count, uint32_t ready_player_count, uint32_t spectator_count);
};
