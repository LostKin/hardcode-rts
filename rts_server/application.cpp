#include "application.h"
#include "network_thread.h"


Application::Application (int& argc, char** argv)
    : QCoreApplication (argc, argv)
{
    network_thread = new NetworkThread ("127.0.0.1", 1331, this);
}
Application::~Application ()
{
    network_thread->wait ();
}

void Application::init ()
{
    network_thread->start ();

    connect (network_thread, &NetworkThread::datagramReceived, this, &Application::datagramHandler);
}
void Application::datagramHandler (QSharedPointer<QNetworkDatagram> datagram)
{
    network_thread->sendDatagram (datagram->makeReply (QByteArray ("RE: ") + datagram->data ()));
}
