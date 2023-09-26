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
    NetworkThread (QObject* parent = nullptr);
    const QString& errorMessage ();
    void sendDatagram (const HCCN::ClientToServer::Message& datagram);

signals:
    void sendDatagramSignal (const HCCN::ClientToServer::Message& datagram);
    void datagramReceived (const QSharedPointer<HCCN::ServerToClient::Message>& datagram);

protected:
    void run () override;

    int return_code = 0;
    QString error_message;

private slots:
    void recieveDatagrams ();
};
