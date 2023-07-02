#pragma once

#include <QThread>
#include <QUdpSocket>
#include <QNetworkDatagram>

class NetworkThread: public QThread
{
    Q_OBJECT

public:
    NetworkThread (const QString& host, quint16 port, QObject* parent = nullptr);
    const QString& errorMessage ();
    void sendDatagram (const QNetworkDatagram& datagram);

signals:
    void sendDatagramSignal (const QNetworkDatagram& datagram);
    void datagramReceived (QSharedPointer<QNetworkDatagram> datagram);

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
