#include "network_manager.h"

#include <QThread>
#include <QUdpSocket>
#include <QDebug>
#include <QCoreApplication>
#include <QNetworkDatagram>

NetworkManager::NetworkManager (const QString& host, quint16 port, QObject* parent)
    : QObject (parent)
    , host (host)
    , port (port)
{
}

bool NetworkManager::start (QString& error_message)
{
    if (!socket.bind (QHostAddress (host), port)) {
        error_message = socket.errorString ();
        return false;
    }
    connect (&socket, &QUdpSocket::readyRead, this, &NetworkManager::recieveDatagrams);
    connect (this, &NetworkManager::sendDatagram, this, &NetworkManager::sendDatagramHandler);
    return true;
}
QSharedPointer<QNetworkDatagram> NetworkManager::takeDatagram ()
{
    QMutexLocker locker (&input_queue_mutex);
    return input_queue.isEmpty () ? nullptr : input_queue.dequeue ();
}
void NetworkManager::recieveDatagrams ()
{
    while (socket.hasPendingDatagrams ()) {
        QSharedPointer<QNetworkDatagram> datagram (new QNetworkDatagram (socket.receiveDatagram ()));
        QMutexLocker locker (&input_queue_mutex);
        input_queue.enqueue (datagram);
    }
    emit datagramsReady ();
}
void NetworkManager::sendDatagramHandler (const QNetworkDatagram& datagram)
{
    // qDebug() << datagram.data() << datagram.destinationAddress();
    int return_code = socket.writeDatagram (datagram);
    // qDebug() << return_code << socket.error() << socket.errorString();
    if (return_code != datagram.data ().size ()) {
        qDebug () << "Datagram size desync" << socket.errorString ();
    }
}
