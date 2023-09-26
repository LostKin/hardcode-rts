#include "room_thread.h"
#include "room.h"

#include <QThread>
#include <QUdpSocket>
#include <QDebug>

RoomThread::RoomThread (const QString& name, QObject* parent)
    : QThread (parent)
    , name_ (name)
{
    room.reset (new Room (this));
    connect (this, &RoomThread::sendRequest, &*room, &Room::receiveRequestHandlerRoom);
    connect (&*room, &Room::sendResponseRoom, this, &RoomThread::sendResponseHandler);
    connect (&*room, &Room::statsUpdated, this, &RoomThread::updateStats);
}

const QString& RoomThread::name () const
{
    return name_;
}
const QString& RoomThread::errorMessage () const
{
    return error_message;
}
quint32 RoomThread::playerCount () const
{
    return player_count;
}
quint32 RoomThread::readyPlayerCount () const
{
    return ready_player_count;
}
quint32 RoomThread::spectatorCount () const
{
    return spectator_count;
}
void RoomThread::run ()
{
    return_code = exec ();
}
void RoomThread::receiveRequestHandler (const RTS::Request& request_oneof, const QSharedPointer<Session>& session, quint64 request_id)
{
    emit sendRequest (request_oneof, session, request_id);
}
void RoomThread::sendResponseHandler (const RTS::Response& response, const QSharedPointer<Session>& session, quint64 request_id)
{
    emit sendResponse (response, session, request_id);
}
void RoomThread::updateStats (quint32 player_count, quint32 ready_player_count, quint32 spectator_count)
{
    this->player_count = player_count;
    this->ready_player_count = ready_player_count;
    this->spectator_count = spectator_count;
}
