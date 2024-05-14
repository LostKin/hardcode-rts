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
    NetworkManager (QObject* parent = nullptr);
    bool start (QString& error_message);
    std::shared_ptr<HCCN::ServerToClient::Message> takeDatagram ();

signals:
    void sendDatagram (const HCCN::ClientToServer::Message& transport_message);
    void datagramsReady ();

private:
    QUdpSocket socket;
    int return_code = 0;
    QHash<HCCN::TransportMessageIdentifier, std::shared_ptr<HCCN::ServerToClient::MessageFragmentCollector>> input_fragment_queue;
    QQueue<std::shared_ptr<HCCN::ServerToClient::Message>> input_queue;
    QMutex input_queue_mutex;

private slots:
    void recieveDatagrams ();
    void sendDatagramHandler (const HCCN::ClientToServer::Message& transport_message);
};
