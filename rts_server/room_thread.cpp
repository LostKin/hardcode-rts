#include "room_thread.h"
#include "room.h"

#include <QThread>
#include <QUdpSocket>
#include <QDebug>

RoomThread::RoomThread(QObject* parent)
    : QThread (parent)
{
    qDebug() << "RoomThread created";
    room.reset(new Room(this));
    connect (this, &RoomThread::sendRequest, &*room, &Room::receiveRequestHandlerRoom);
    connect (&*room, &Room::sendResponseRoom, this, &RoomThread::sendResponseHandler);
}

void RoomThread::run ()
{
    qDebug() << "RoomThread running";
    //Room room;

   return_code = exec ();
}

void RoomThread::receiveRequestHandler (RTS::Request request_oneof, QSharedPointer<Session> session) {
    //qDebug() << "receiveRequestHandler started";
    emit this->sendRequest(request_oneof, session);
    //qDebug() << "receiveRequestHandler ended";
    //room->receiveREquestHandler(request_oneof, session);
}


void RoomThread::sendResponseHandler (RTS::Response response, QSharedPointer<Session> session) {
    //qDebug() << "sendResponseHandler started";
    emit sendResponse(response, session);
}