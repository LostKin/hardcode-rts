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
}

void Room::setError (RTS::Error* error, const std::string& error_message, RTS::ErrorCode error_code)
{
    error->set_message (error_message);
    error->set_code (error_code);
}

void Room::receiveRequestHandlerRoom(RTS::Request request_oneof, QSharedPointer<Session> session) {
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
            red_ready = true;
            qDebug() << "red ready";
        }

        if (session->current_team == RTS::Team::BLUE) {
            blue_ready = true;
            qDebug() << "blue ready";
        }
        if (blue_ready && red_ready) {
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
    }
}

void Room::readyHandler() {
    qDebug() << "readyHandler started";
    RTS::Response response_oneof;
    RTS::MatchStartResponse* response = response_oneof.mutable_match_start ();
    emit sendResponseRoom(response_oneof, red_team);
    emit sendResponseRoom(response_oneof, blue_team);
    return;
}