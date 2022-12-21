#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QThread>
#include ".proto_stubs/session_level.pb.h"

void waitTillReception(QUdpSocket& socket) {
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
    t.mutable_auth()->set_login("login");
    t.mutable_auth()->set_password("a");
    qDebug() << t.auth().login().data() << t.auth().password().data();
    //QByteArray msg;
    std::string msg;
    t.SerializeToString(&msg);
    qint64 ret = socket.writeDatagram (msg.data(), msg.size(), QHostAddress ("127.0.0.1"), 1331);
    qDebug () << ret << "bytes sent";
    qDebug () << "awaiting response for 3 seconds";

    waitTillReception(socket);

    QNetworkDatagram datagram = socket.receiveDatagram ();
    qDebug () << datagram.data(); 
    RTS::Response res;
    QByteArray data = datagram.data();
    res.ParseFromArray(data.data(), data.size());
    
    int token = -1;

    switch (res.message_case()) {
        case RTS::Response::MessageCase::kAuth:
        {
            qDebug() << res.auth().token().token();
            token = res.auth().token().token();
        }
        break;
    }

    QNetworkInterface interface = QNetworkInterface::interfaceFromIndex(datagram.interfaceIndex());
    qDebug () << "Sender:" << datagram.senderAddress() << ":" << datagram.senderPort ();
    qDebug () << "Receiver:" << datagram.destinationAddress() << ":" << datagram.destinationPort ();
    qDebug () << "Response body:" << datagram.data ();
    qDebug () << "Interface MTU:" << interface.maximumTransmissionUnit();

    qDebug() << token;

    if (token != -1) {
        qDebug() << "About to join the room";
        RTS::Request t2;
        t2.mutable_room()->mutable_token()->set_token(token);
        t2.mutable_room()->set_room(0);
        //t2.set_allocated_room(room); 


        t2.SerializeToArray(&msg, 0);
        
        
        qint64 ret = socket.writeDatagram (msg.data(), msg.size(), QHostAddress ("127.0.0.1"), 1331);
        qDebug () << ret << "bytes sent";
        qDebug () << "awaiting response";
        
        waitTillReception(socket);

        QNetworkDatagram datagram = socket.receiveDatagram ();
        qDebug () << datagram.data(); 
        QByteArray data = datagram.data();
        RTS::Response res;
        res.ParseFromArray(data.data(), data.size());
    }

    return 0;
}
