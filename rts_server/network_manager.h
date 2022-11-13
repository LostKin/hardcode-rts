#pragma once

#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QQueue>
#include <QMutex>


class NetworkManager: public QObject
{
    Q_OBJECT

public:
    NetworkManager (const QString& host, quint16 port, QObject *parent = nullptr);
    bool start (QString& error_message);
    QSharedPointer<QNetworkDatagram> takeDatagram ();

signals:
    void sendDatagram (QNetworkDatagram datagram);
    void datagramsReady ();

private:
    const QString host;
    const quint16 port;

    QUdpSocket socket;
    int return_code = 0;
    QQueue<QSharedPointer<QNetworkDatagram>> input_queue;
    QMutex input_queue_mutex;

private slots:
    void recieveDatagrams ();
    void sendDatagramHandler (QNetworkDatagram datagram);
};
