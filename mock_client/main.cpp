#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QThread>
#include ".proto_stubs/session_level.pb.h"

void waitTillReception (QUdpSocket& socket)
{
    for (size_t i = 0; i < 3000; ++i) {
        if (socket.hasPendingDatagrams ())
            break;
        QThread::msleep (1);
    }
}

int main ()
{
    QUdpSocket socket;
    socket.bind (QHostAddress ("127.0.0.1"));
    RTS::Request t;
    t.mutable_join_team ()->mutable_session_token ()->set_value (1);
    t.mutable_join_team ()->mutable_request_token ()->set_value (1);
    t.mutable_join_team ()->set_team_id (1);
    std::string msg;
    t.SerializeToString (&msg);
    qint64 ret = socket.writeDatagram (msg.data (), msg.size (), QHostAddress ("127.0.0.1"), 1331);
    waitTillReception (socket);
    QNetworkDatagram datagram = socket.receiveDatagram ();
    qDebug () << datagram.data ();
    return 0;
}
