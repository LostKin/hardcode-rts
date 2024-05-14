#pragma once

#include "client_to_server.h"
#include "server_to_client.h"

#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QQueue>
#include <QMutex>


class NetworkManager: public QObject
{
    Q_OBJECT

public:
    NetworkManager (const std::string& host, uint16_t port, QObject* parent = nullptr);
    bool start (std::string& error_message);
    std::shared_ptr<HCCN::ClientToServer::Message> takeDatagram ();

signals:
    void sendDatagram (const std::shared_ptr<HCCN::ServerToClient::Message>& datagram);
    void datagramsReady ();

private slots:
    void recieveDatagrams ();
    void sendDatagramHandler (const std::shared_ptr<HCCN::ServerToClient::Message>& datagram);

private:
    const std::string host;
    const uint16_t port;

    QUdpSocket socket;
    int return_code = 0;
    QHash<HCCN::TransportMessageIdentifier, std::shared_ptr<HCCN::ClientToServer::MessageFragmentCollector>> input_fragment_queue;
    QQueue<std::shared_ptr<HCCN::ClientToServer::Message>> input_queue;
    QMutex input_queue_mutex;
};
