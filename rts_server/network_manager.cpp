#include "network_manager.h"

#include <QThread>
#include <QUdpSocket>
#include <QCoreApplication>
#include <QNetworkDatagram>

NetworkManager::NetworkManager (const QString& host, uint16_t port, QObject* parent)
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
QSharedPointer<HCCN::ClientToServer::Message> NetworkManager::takeDatagram ()
{
    QMutexLocker locker (&input_queue_mutex);
    return input_queue.isEmpty () ? nullptr : input_queue.dequeue ();
}
void NetworkManager::recieveDatagrams ()
{
    while (socket.hasPendingDatagrams ()) {
        QNetworkDatagram datagram = socket.receiveDatagram ();
        if (QSharedPointer<HCCN::ClientToServer::MessageFragment> message_fragment = HCCN::ClientToServer::MessageFragment::parse (datagram)) {
            QMutexLocker locker (&input_queue_mutex);
            HCCN::TransportMessageIdentifier transport_message_identifier (message_fragment->host, message_fragment->port, message_fragment->request_id);
            QHash<HCCN::TransportMessageIdentifier, QSharedPointer<HCCN::ClientToServer::MessageFragmentCollector>>::iterator fragment_collector_it =
                input_fragment_queue.find (transport_message_identifier);
            if (fragment_collector_it != input_fragment_queue.end ()) {
                if (*fragment_collector_it) {
                    HCCN::ClientToServer::MessageFragmentCollector& fragment_collector = **fragment_collector_it;
                    fragment_collector.insert (message_fragment);
                    if (fragment_collector.complete ()) {
                        input_queue.enqueue (fragment_collector.build ());
                        fragment_collector_it->reset ();
                    }
                }
            } else {
                QSharedPointer<HCCN::ClientToServer::MessageFragmentCollector> fragment_collector (new HCCN::ClientToServer::MessageFragmentCollector);
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
void NetworkManager::sendDatagramHandler (const QSharedPointer<HCCN::ServerToClient::Message>& message)
{
    QVector<QNetworkDatagram> datagrams = message->encode ();
    for (const QNetworkDatagram& datagram: datagrams)
        socket.writeDatagram (datagram);
}
