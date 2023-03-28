#include "room.h"

#include <QThread>
#include <QUdpSocket>
#include <QDebug>
#include <QCoreApplication>
#include <QNetworkDatagram>
#include <QTimer>

Room::Room (QObject* parent)
    : QObject (parent)
{
    timer.reset (new QTimer(this));
    connect (&*timer, &QTimer::timeout, this, &Room::tick);
}

void Room::tick () {
    match_state->tick();
    RTS::Response response_oneof_red, response_oneof_blue;
    RTS::MatchState* response_red = response_oneof_red.mutable_match_state ();
    RTS::MatchState* response_blue = response_oneof_blue.mutable_match_state ();
    for (QHash<quint32, Unit>::const_iterator iter = match_state->unitsRef().cbegin(); iter != match_state->unitsRef().cend(); iter++) {
        RTS::Unit* unit_red = response_red->add_units();
        RTS::Unit* unit_blue = response_blue->add_units();
        if (iter->team == Unit::Team::Red) {
            unit_red->set_team(RTS::Team::RED);
            unit_blue->set_team(RTS::Team::RED);
            QMap<quint32, quint32>::const_iterator it = client_to_server.find(iter.key());
            if (it != client_to_server.cend()) {
                unit_red->mutable_client_id()->set_id(it.key());
            }
        }
        if (iter->team == Unit::Team::Blue) {
            unit_red->set_team(RTS::Team::BLUE);
            unit_blue->set_team(RTS::Team::RED);
            QMap<quint32, quint32>::const_iterator it = client_to_server.find(iter.key());
            if (it != client_to_server.cend()) {
                unit_blue->mutable_client_id()->set_id(it.key());
            }
        }
        switch (iter->type) {
        case Unit::Type::Seal:
        {
            unit_red->set_type(RTS::UnitType::SEAL);
            unit_blue->set_type(RTS::UnitType::SEAL);
        } break;
        case Unit::Type::Crusader:
        {
            unit_red->set_type(RTS::UnitType::CRUSADER);
            unit_blue->set_type(RTS::UnitType::CRUSADER);
        } break;
        }
        unit_red->mutable_position()->set_x(iter->position.x());
        unit_red->mutable_position()->set_y(iter->position.y());
        unit_red->set_orientation(iter->orientation);
        unit_red->set_id(iter.key());
        unit_blue->mutable_position()->set_x(iter->position.x());
        unit_blue->mutable_position()->set_y(iter->position.y());
        unit_blue->set_orientation(iter->orientation);
        unit_blue->set_id(iter.key());
    }
    //response->units;
    emit sendResponseRoom(response_oneof_red, red_team);
    emit sendResponseRoom(response_oneof_blue, blue_team);
}

void Room::setError (RTS::Error* error, const std::string& error_message, RTS::ErrorCode error_code)
{
    error->set_message (error_message);
    error->set_code (error_code);
}

void Room::init_matchstate () {
    match_state.reset(new MatchState());
    {
        match_state->createUnit (Unit::Type::Crusader, Unit::Team::Red, QPointF (-15, -7), 0);
        match_state->createUnit (Unit::Type::Seal, Unit::Team::Red, QPointF (1, 3), 0);
        match_state->createUnit (Unit::Type::Seal, Unit::Team::Red, QPointF (8, 3), 0);
        match_state->createUnit (Unit::Type::Seal, Unit::Team::Red, QPointF (8, 6), 0);
        match_state->createUnit (Unit::Type::Seal, Unit::Team::Red, QPointF (8, 9), 0);
        match_state->createUnit (Unit::Type::Seal, Unit::Team::Blue, QPointF (10, 5), 0);
        match_state->createUnit (Unit::Type::Crusader, Unit::Team::Blue, QPointF (-20, -8), 0);
    }
}

void Room::receiveRequestHandlerRoom (RTS::Request request_oneof, QSharedPointer<Session> session) {
    //qDebug() << "Room receive request started";
    switch (request_oneof.message_case ()) {
    case RTS::Request::MessageCase::kJoinTeam: {
        const RTS::JoinTeamRequest& request = request_oneof.join_team ();

        if (session->current_team.has_value()) {
            // error - player already has a team
            RTS::Response response_oneof;
            RTS::JoinTeamResponse* response = response_oneof.mutable_join_team ();
            response->mutable_request_token ()->set_value (request_oneof.join_team().request_token().value());
            setError(response->mutable_error(), "Already joined a team", RTS::ALREADY_JOINED_TEAM);
            emit sendResponseRoom(response_oneof, session);
            return;
        }

        if (request.team() == RTS::Team::RED) {
            if (!red_team.isNull()) {
                RTS::Response response_oneof;
                RTS::JoinTeamResponse* response = response_oneof.mutable_join_team ();
                response->mutable_request_token ()->set_value (request_oneof.join_team().request_token().value());
                setError(response->mutable_error(), "Team already taken", RTS::TEAM_ALREADY_TAKEN);
                emit sendResponseRoom(response_oneof, session);
                return;
            }
            session->current_team = RTS::Team::RED;
            red_team = session;
        }
        if (request.team() == RTS::Team::BLUE) {
            if (!blue_team.isNull()) {
                RTS::Response response_oneof;
                RTS::JoinTeamResponse* response = response_oneof.mutable_join_team ();
                response->mutable_request_token ()->set_value (request_oneof.join_team().request_token().value());
                setError(response->mutable_error(), "Team already taken", RTS::TEAM_ALREADY_TAKEN);
                emit sendResponseRoom(response_oneof, session);
                return;
            }
            session->current_team = RTS::Team::BLUE;
            blue_team = session;
        }
        qDebug() << "Successfully joined team";
        //teams[request.team_id()] = session;
        RTS::Response response_oneof;
        RTS::JoinTeamResponse* response = response_oneof.mutable_join_team ();
        response->mutable_request_token ()->set_value (request_oneof.join_team().request_token().value());
        response->mutable_success ();
        emit sendResponseRoom(response_oneof, session);
        return;
    } break;
    case RTS::Request::MessageCase::kReady: {
        /*const RTS::ReadyRequest& request = request_oneof.ready ();
        if (teams[request.team_id()] == 0, ) {
            RTS::Response response_oneof;
            RTS::JoinTeamResponse* response = response_oneof.mutable_join_team ();
            response->mutable_request_token ()->set_value (request_oneof.join_team().request_token().value());
            setError(response->mutable_error(), "Team already taken", RTS::TEAM_ALREADY_TAKEN);
            emit sendResponseRoom(response_oneof, session);
            return;
        }
        RTS::Response response_oneof;
        RTS::ReadyResponse* response = response_oneof.mutable_join_team ();
        response->mutable_request_token ()->set_value (request_oneof.join_team().request_token().value());
        response->mutable_success ();
        emit sendResponseRoom(response_oneof, session);*/
        if (session->current_team == RTS::Team::RED) {
            qDebug() << "red ready";
        }

        if (session->current_team == RTS::Team::BLUE) {
            qDebug() << "blue ready";
        }
        session->ready = true;
        if (!red_team.isNull() && !blue_team.isNull() && red_team->ready && blue_team->ready) {
            qDebug() << "both ready";
            RTS::Response response_oneof;
            RTS::ReadyResponse* response = response_oneof.mutable_ready ();
            response->mutable_success ();
            emit sendResponseRoom(response_oneof, red_team);
            emit sendResponseRoom(response_oneof, blue_team);
            QTimer::singleShot(5000, this, &Room::readyHandler);
        }
        return;
    } break;
    case RTS::Request::MessageCase::kUnitCreate: {
        const RTS::UnitCreateRequest& request = request_oneof.unit_create ();
        Unit::Team team = Unit::Team::Blue;
        if (session->current_team == RTS::Team::RED) {
            team = Unit::Team::Red;
        }
        Unit::Type type;
        switch (request.unit_type()) {
        case RTS::UnitType::SEAL: {
            type = Unit::Type::Seal;
        } break;
        case RTS::UnitType::CRUSADER: {
            type = Unit::Type::Crusader;
        } break;
        }
        QHash<quint32, Unit>::iterator unit = match_state->createUnit (type, team, QPointF (request.position().x(), request.position().y()), 0);
        client_to_server[request.id()] = unit.key();
    }
    }
}

void Room::readyHandler() {
    init_matchstate();
    const quint32 TICK_LENGTH = 20;
    timer->start(TICK_LENGTH);
    qDebug() << "readyHandler started";
    RTS::Response response_oneof;
    RTS::MatchStartResponse* response = response_oneof.mutable_match_start ();
    emit sendResponseRoom(response_oneof, red_team);
    emit sendResponseRoom(response_oneof, blue_team);
    return;
}