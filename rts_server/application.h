#pragma once

#include <QCoreApplication>
#include <QSharedPointer>
#include <QNetworkDatagram>

class NetworkThread;

class Application: public QCoreApplication
{
    Q_OBJECT

public:
    Application (int& argc, char** argv);
    ~Application ();

    void init ();

private slots:
    void datagramHandler (QSharedPointer<QNetworkDatagram> datagram);

private:
    NetworkThread* network_thread;
};
