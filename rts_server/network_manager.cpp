#include "network_manager.h"

#include <QThread>
#include <QUdpSocket>
#include <QCoreApplication>
#include <QNetworkDatagram>

NetworkManager::NetworkManager (const std::string& host, uint16_t port, QObject* parent)
    : QObject (parent)
    , host (host)
    , port (port)
{
}

bool NetworkManager::start (std::string& error_message)
{
    if (!socket.bind (QHostAddress (QString::fromStdString (host)), port)) {
        error_message = socket.errorString ().toStdString ();
        return false;
    }
    connect (&socket, &QUdpSocket::readyRead, this, &NetworkManager::recieveDatagrams);
    connect (this, &NetworkManager::sendDatagram, this, &NetworkManager::sendDatagramHandler);
    return true;
}
std::shared_ptr<HCCN::ClientToServer::Message> NetworkManager::takeDatagram ()
{
    QMutexLocker locker (&input_queue_mutex);
    return input_queue.isEmpty () ? nullptr : input_queue.dequeue ();
}
void NetworkManager::recieveDatagrams ()
{
    while (socket.hasPendingDatagrams ()) {
        QNetworkDatagram datagram = socket.receiveDatagram ();
        if (std::shared_ptr<HCCN::ClientToServer::MessageFragment> message_fragment = HCCN::ClientToServer::MessageFragment::parse (datagram)) {
            QMutexLocker locker (&input_queue_mutex);
            HCCN::TransportMessageIdentifier transport_message_identifier (message_fragment->host, message_fragment->port, message_fragment->request_id);
            QHash<HCCN::TransportMessageIdentifier, std::shared_ptr<HCCN::ClientToServer::MessageFragmentCollector>>::iterator fragment_collector_it =
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
                std::shared_ptr<HCCN::ClientToServer::MessageFragmentCollector> fragment_collector (new HCCN::ClientToServer::MessageFragmentCollector);
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
void NetworkManager::sendDatagramHandler (const std::shared_ptr<HCCN::ServerToClient::Message>& message)
{
    std::vector<QNetworkDatagram> datagrams = message->encode ();
    for (const QNetworkDatagram& datagram: datagrams)
        socket.writeDatagram (datagram);
}
