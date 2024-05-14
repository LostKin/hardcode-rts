#pragma once

#include "client_to_server.h"
#include "server_to_client.h"

#include <QThread>
#include <QUdpSocket>
#include <QNetworkDatagram>


class NetworkThread: public QThread
{
    Q_OBJECT

public:
    NetworkThread (const std::string& host, uint16_t port, QObject* parent = nullptr);
    const std::string& errorMessage ();
    void sendDatagram (const std::shared_ptr<HCCN::ServerToClient::Message>& datagram);

signals:
    void sendDatagramSignal (const std::shared_ptr<HCCN::ServerToClient::Message>& datagram);
    void datagramReceived (const std::shared_ptr<HCCN::ClientToServer::Message>& datagram);

protected:
    void run () override;

private:
    const std::string host;
    const uint16_t port;

    int return_code = 0;
    std::string error_message;

private slots:
    void recieveDatagrams ();
};
