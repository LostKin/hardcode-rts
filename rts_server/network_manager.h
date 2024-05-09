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
    NetworkManager (const QString& host, uint16_t port, QObject* parent = nullptr);
    bool start (QString& error_message);
    QSharedPointer<HCCN::ClientToServer::Message> takeDatagram ();

signals:
    void sendDatagram (const QSharedPointer<HCCN::ServerToClient::Message>& datagram);
    void datagramsReady ();

private slots:
    void recieveDatagrams ();
    void sendDatagramHandler (const QSharedPointer<HCCN::ServerToClient::Message>& datagram);

private:
    const QString host;
    const uint16_t port;

    QUdpSocket socket;
    int return_code = 0;
    QHash<HCCN::TransportMessageIdentifier, QSharedPointer<HCCN::ClientToServer::MessageFragmentCollector>> input_fragment_queue;
    QQueue<QSharedPointer<HCCN::ClientToServer::Message>> input_queue;
    QMutex input_queue_mutex;
};
