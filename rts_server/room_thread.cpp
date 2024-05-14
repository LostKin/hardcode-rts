#include "room_thread.h"
#include "room.h"

#include <QThread>
#include <QUdpSocket>


RoomThread::RoomThread (const std::string& name, QObject* parent)
    : QThread (parent)
    , name_ (name)
{
    room.reset (new Room (this));
    connect (this, &RoomThread::sendRequest, &*room, &Room::receiveRequestHandlerRoom);
    connect (&*room, &Room::sendResponseRoom, this, &RoomThread::sendResponseHandler);
    connect (&*room, &Room::statsUpdated, this, &RoomThread::updateStats);
}

const std::string& RoomThread::name () const
{
    return name_;
}
const std::string& RoomThread::errorMessage () const
{
    return error_message;
}
uint32_t RoomThread::playerCount () const
{
    return player_count;
}
uint32_t RoomThread::readyPlayerCount () const
{
    return ready_player_count;
}
uint32_t RoomThread::spectatorCount () const
{
    return spectator_count;
}
void RoomThread::run ()
{
    return_code = exec ();
}
void RoomThread::receiveRequestHandler (const RTS::Request& request_oneof, const std::shared_ptr<Session>& session, uint64_t request_id)
{
    emit sendRequest (request_oneof, session, request_id);
}
void RoomThread::sendResponseHandler (const RTS::Response& response, const std::shared_ptr<Session>& session, uint64_t request_id)
{
    emit sendResponse (response, session, request_id);
}
void RoomThread::updateStats (uint32_t player_count, uint32_t ready_player_count, uint32_t spectator_count)
{
    this->player_count = player_count;
    this->ready_player_count = ready_player_count;
    this->spectator_count = spectator_count;
}
