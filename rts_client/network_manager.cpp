#include "network_manager.h"

#include <QThread>
#include <QUdpSocket>
#include <QCoreApplication>
#include <QNetworkDatagram>

NetworkManager::NetworkManager (QObject* parent)
    : QObject (parent)
{
}

bool NetworkManager::start (QString& error_message)
{
    if (!socket.bind (QHostAddress (QHostAddress::Any), 0)) {
        error_message = socket.errorString ();
        return false;
    }
    connect (&socket, &QUdpSocket::readyRead, this, &NetworkManager::recieveDatagrams);
    connect (this, &NetworkManager::sendDatagram, this, &NetworkManager::sendDatagramHandler);
    return true;
}
QSharedPointer<HCCN::ServerToClient::Message> NetworkManager::takeDatagram ()
{
    QMutexLocker locker (&input_queue_mutex);
    return input_queue.isEmpty () ? nullptr : input_queue.dequeue ();
}
void NetworkManager::recieveDatagrams ()
{
    while (socket.hasPendingDatagrams ()) {
        QNetworkDatagram datagram = socket.receiveDatagram ();
        if (QSharedPointer<HCCN::ServerToClient::MessageFragment> message_fragment = HCCN::ServerToClient::MessageFragment::parse (datagram)) {
            QMutexLocker locker (&input_queue_mutex);
            HCCN::TransportMessageIdentifier transport_message_identifier (message_fragment->host, message_fragment->port, message_fragment->response_id);
            QHash<HCCN::TransportMessageIdentifier, QSharedPointer<HCCN::ServerToClient::MessageFragmentCollector>>::iterator fragment_collector_it =
                input_fragment_queue.find (transport_message_identifier);
            if (fragment_collector_it != input_fragment_queue.end ()) {
                if (*fragment_collector_it) {
                    HCCN::ServerToClient::MessageFragmentCollector& fragment_collector = **fragment_collector_it;
                    fragment_collector.insert (message_fragment);
                    if (fragment_collector.complete ()) {
                        input_queue.enqueue (fragment_collector.build ());
                        fragment_collector_it->reset ();
                    }
                }
            } else {
                QSharedPointer<HCCN::ServerToClient::MessageFragmentCollector> fragment_collector (new HCCN::ServerToClient::MessageFragmentCollector);
                fragment_collector->insert (message_fragment);
                if (fragment_collector->complete ()) {
                    input_queue.enqueue (fragment_collector->build ());
                    fragment_collector.reset ();
                }
                input_fragment_queue.insert (transport_message_identifier, fragment_collector);
            }
        }
    }
    emit datagramsReady ();
}
void NetworkManager::sendDatagramHandler (const HCCN::ClientToServer::Message& transport_message)
{
    QVector<QNetworkDatagram> datagrams = transport_message.encode ();
    for (const QNetworkDatagram& datagram: datagrams)
        socket.writeDatagram (datagram);
}
