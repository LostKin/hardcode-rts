#pragma once

#include ".proto_stubs/entities.pb.h"
#include "matchstate.h"

#include <QNetworkDatagram>

// TODO: Locking sessions
struct Session {
    Session () = delete;
    Session (quint64 session_id)
        : session_id (session_id)
    {
    }
    Session (QHostAddress client_address, quint16 client_port, QByteArray login, quint64 session_id)
        : client_address (client_address)
        , client_port (client_port)
        , login (login)
        , session_id (session_id)
    {
    }

    QHostAddress client_address;
    quint16 client_port;
    QByteArray login;
    quint64 session_id;
    std::optional<quint32> current_room = {};
    RTS::Role current_role = RTS::ROLE_UNSPECIFIED;
    std::optional<Unit::Team> current_team = {};
    bool query_room_list_requested = false;
    bool ready = false;
};
