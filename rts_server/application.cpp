#include "application.h"
#include "network_thread.h"
#include ".proto_stubs/session_level.pb.h"
#include <QFile>


/*SessionInfo::SessionInfo(int32_t id, std::string _login) {
    current_room = id; 
    login = _login;
}*/

Room::Room() {}

Application::Application (int& argc, char** argv)
    : QCoreApplication (argc, argv)
{
    token = 0;
    MOD = 1e18;
    network_thread = new NetworkThread ("127.0.0.1", 1331, this);
    room.network_thread = new NetworkThread ("127.0.0.1", 1332, this);
    room.id = 0;
}

uint64_t Application::next_token() {
    ++token;
    return token;
}

Application::~Application ()
{
    network_thread->wait ();
}

bool Application::init ()
{
    const QString fname ("users.txt");
    QFile file (fname);
    if (!file.open (QIODevice::ReadOnly)) {
        qDebug () << "Failed to open file" << fname << ":" << file.errorString ();
        return false;
    }
    while (!file.atEnd ()) {
        QByteArray line = file.readLine ();
        if (!line.size ()) {
            if (file.atEnd ())
                break;
            qDebug () << "Failed to read from file" << fname << ":" << file.errorString ();
            return false;
        }
        if (line[line.size () - 1] == '\n')
            line.resize (line.size () - 1);
        QList<QByteArray> fields = line.split (':');
        if (fields.size () != 2) {
            qDebug () << "Invalid line" << line << "at" << fname << ":" << "2 fields expected";
            return false;
        }
        users[fields.at (0)] = fields.at (1);
        qDebug () << fields.at (0) << "->" << fields.at (1);
    }
    //users["login"] = "a";
    //qDebug() << test << "\n";
    //qDebug() << users[test].data();
    network_thread->start ();

    connect (network_thread, &NetworkThread::datagramReceived, this, &Application::sessionDatagramHandler);
    return true;
}

void Application::sendReply(QSharedPointer<QNetworkDatagram> datagram, const std::string& msg) {
    network_thread->sendDatagram (QNetworkDatagram (msg.data(), datagram->senderAddress (), datagram->senderPort ()));
}

void Application::sessionDatagramHandler (QSharedPointer<QNetworkDatagram> datagram)
{       
    //if datagram->data() is AuthorizationRequest
    if (true) {
        //datagram->data() -> AutorizationRequest
        RTS::Request interaction;
        
        QByteArray data = datagram->data();

        interaction.ParseFromArray(data.data(), data.size());
        
        //qDebug() << interaction.message_case();

        switch(interaction.message_case()) {
            case RTS::Request::MessageCase::kAuth:
                {
                qDebug() << "Authorization request";
                const RTS::AuthorizationRequest &auth = interaction.auth();
                //req = interaction.req();
                //qDebug() << "datagram : " << datagram->data();
                //qDebug() << "found login :" << req.login().data() << ", found password :" << req.password().data();
                //qDebug() << "password required :" << users[req.login()].data();
                QByteArray login, password;
                login = QByteArray::fromStdString(auth.login());
                password = QByteArray::fromStdString(auth.password());
                qDebug() << auth.login().data() << auth.password().data();
                if (users.find(login) != users.end() && users[login] == password) {
                    qDebug() << "EQUAL";
                 //  if (login_tokens.find(req.login()) == login_tokens.end()) {
                        RTS::Response resp;
                        resp.mutable_auth()->mutable_token()->set_token(next_token());

                        //info[token->token()] = {-1, req.login()};
                        info[resp.mutable_auth()->mutable_token()->token()] = {-1};
                        login_tokens[login] = resp.mutable_auth()->mutable_token()->token();

                        std::string msg;
                        resp.SerializeToString(&msg);
                        //network_thread->sendDatagram (datagram->makeReply (msg.data()));
                        sendReply(datagram, msg);
                        qDebug() << "Response sent";
                        //Log user in : AuthorizatonResponse
                 /*   } else {
                        qDebug() << "Already logged in";
                        RTS::AuthorizationResponse resp = RTS::AuthorizationResponse();
                        RTS::Error* error = new RTS::Error();
                        std::string message = "user already logged in";
                        error->set_message(message);
                        error->set_code(1);
                        resp.set_allocated_error(error);
                        std::string msg;
                        resp.SerializeToString(&msg);
                        //qDebug() << req.password().size() << users["login"].size(); 
                        //Do not log user in : AuthorizationResponse
                        network_thread->sendDatagram (datagram->makeReply (msg.data()));
                        qDebug() << "Response sent";
                    }
                 */
                } else {
                    qDebug() << "NOT EQUAL";
                    qDebug() << login << password;
                    RTS::Response resp;
                    
                    resp.mutable_auth()->mutable_error()->set_message("wrong login or password");
                    resp.mutable_auth()->mutable_error()->set_code(1);
                    
                    std::string msg;
                    resp.SerializeToString(&msg);
                    //qDebug() << req.password().size() << users["login"].size(); 
                    //Do not log user in : AuthorizationResponse
                    sendReply(datagram, msg);
                    qDebug() << "Response sent";
                };
                }
                break;
            case RTS::Request::MessageCase::kRoom:
                {
                qDebug() << "Room join request";
                const RTS::EnterRoomRequest &room = interaction.room(); 
                if (info[room.token().token()].current_room != -1) {
                    qDebug() << "Already joined a room";
                    RTS::Response resp;
                    resp.mutable_room()->mutable_error()->set_message("Already joined a room");
                    resp.mutable_room()->mutable_error()->set_code(1);
                    std::string msg;
                    resp.SerializeToString(&msg);
                    //qDebug() << req.password().size() << users["login"].size(); 
                    //Do not log user in : AuthorizationResponse
                    sendReply(datagram, msg);
                    qDebug() << "Response sent";
                } else {
                    qDebug() << "Join successfull";
                    RTS::Response resp;
                    resp.mutable_room()->mutable_token()->set_token(next_token());
                
                    qDebug() << resp.mutable_room()->mutable_token()->token();

                    
                    //info[token->token()] = {req.room()};
                    std::string msg2;
                    resp.SerializeToString(&msg2);
                    
                    qDebug() << resp.room().token().token();
                    
                    qDebug() << msg2.data();

                    sendReply(datagram, msg2);
                    qDebug() << "Response sent";
                }
                }
                break;
        }


        
    }
    //network_thread->sendDatagram (datagram->makeReply (QByteArray ("RE: ") + datagram->data ()));
}
