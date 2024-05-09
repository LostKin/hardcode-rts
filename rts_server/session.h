#pragma once

#include ".proto_stubs/entities.pb.h"
#include "matchstate.h"

#include <QNetworkDatagram>

// TODO: Locking sessions
struct Session {
    Session () = delete;
    Session (uint64_t session_id)
        : session_id (session_id)
    {
    }
    Session (QHostAddress client_address, uint16_t client_port, QByteArray login, uint64_t session_id)
        : client_address (client_address)
        , client_port (client_port)
        , login (login)
        , session_id (session_id)
    {
    }

    QHostAddress client_address;
    uint16_t client_port;
    QByteArray login;
    uint64_t session_id;
    std::optional<uint32_t> current_room = {};
    RTS::Role current_role = RTS::ROLE_UNSPECIFIED;
    std::optional<Unit::Team> current_team = {};
    bool query_room_list_requested = false;
    bool ready = false;
};
