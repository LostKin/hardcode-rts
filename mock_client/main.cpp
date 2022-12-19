#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QThread>


int main ()
{
    QUdpSocket socket;
    socket.bind (QHostAddress ("127.0.0.1"));
    char msg[] = "Hello, server";
    qint64 ret = socket.writeDatagram (msg, sizeof (msg) - sizeof (""), QHostAddress ("127.0.0.1"), 1331);
    qDebug () << ret << "bytes sent";
    qDebug () << "awaiting response for 3 seconds";
    for (size_t i = 0; i < 3000; ++i) {
        if (socket.hasPendingDatagrams ())
            break;
        QThread::msleep (1);
    }
    QNetworkDatagram datagram = socket.receiveDatagram ();
    if (!datagram.isValid ()) {
        qDebug () << "Failed to recieve datagram";
        return 1;
    }
    QNetworkInterface interface = QNetworkInterface::interfaceFromIndex (datagram.interfaceIndex ());
    qDebug () << "Sender:" << datagram.senderAddress () << ":" << datagram.senderPort ();
    qDebug () << "Receiver:" << datagram.destinationAddress () << ":" << datagram.destinationPort ();
    qDebug () << "Response body:" << datagram.data ();
    qDebug () << "Interface MTU:" << interface.maximumTransmissionUnit ();

    return 0;
}
