#pragma once

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
    QSharedPointer<QNetworkDatagram> takeDatagram ();

signals:
    void sendDatagram (const QNetworkDatagram& datagram);
    void datagramsReady ();

private:
    QUdpSocket socket;
    int return_code = 0;
    QQueue<QSharedPointer<QNetworkDatagram>> input_queue;
    QMutex input_queue_mutex;

private slots:
    void recieveDatagrams ();
    void sendDatagramHandler (const QNetworkDatagram& datagram);
};
