#include "network_thread.h"

#include "network_manager.h"

#include <QThread>
#include <QUdpSocket>

NetworkThread::NetworkThread (QObject* parent)
    : QThread (parent)
{
}

const QString& NetworkThread::errorMessage ()
{
    return error_message;
}
void NetworkThread::sendDatagram (const QNetworkDatagram& datagram)
{
    emit sendDatagramSignal (datagram);
}
void NetworkThread::run ()
{
    NetworkManager network_manager;
    if (!network_manager.start (error_message)) {
        return_code = 1;
        return;
    }
    connect (this, &NetworkThread::sendDatagramSignal, &network_manager, &NetworkManager::sendDatagram, Qt::QueuedConnection);
    connect (&network_manager, &NetworkManager::datagramsReady, this, &NetworkThread::recieveDatagrams, Qt::QueuedConnection);
    return_code = exec ();
}
void NetworkThread::recieveDatagrams ()
{
    NetworkManager* network_manager = (NetworkManager*) sender ();
    while (QSharedPointer<QNetworkDatagram> datagram = network_manager->takeDatagram ()) {
        emit datagramReceived (datagram);
    }
}
