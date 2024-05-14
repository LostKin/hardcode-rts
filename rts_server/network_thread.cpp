#include "network_thread.h"

#include "network_manager.h"

#include <QThread>
#include <QUdpSocket>


NetworkThread::NetworkThread (const std::string& host, uint16_t port, QObject* parent)
    : QThread (parent)
    , host (host)
    , port (port)
{
}

const std::string& NetworkThread::errorMessage ()
{
    return error_message;
}
void NetworkThread::sendDatagram (const std::shared_ptr<HCCN::ServerToClient::Message>& datagram)
{
    emit sendDatagramSignal (datagram);
}
void NetworkThread::run ()
{
    NetworkManager network_manager (host, port);
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
    while (std::shared_ptr<HCCN::ClientToServer::Message> network_message = network_manager->takeDatagram ())
        emit datagramReceived (network_message);
}
