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
    NetworkThread (const QString& host, quint16 port, QObject* parent = nullptr);
    const QString& errorMessage ();
    void sendDatagram (const QSharedPointer<HCCN::ServerToClient::Message>& datagram);

signals:
    void sendDatagramSignal (const QSharedPointer<HCCN::ServerToClient::Message>& datagram);
    void datagramReceived (const QSharedPointer<HCCN::ClientToServer::Message>& datagram);

protected:
    void run () override;

private:
    const QString host;
    const quint16 port;

    int return_code = 0;
    QString error_message;

private slots:
    void recieveDatagrams ();
};
