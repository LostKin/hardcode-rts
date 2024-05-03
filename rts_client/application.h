#pragma once

#include "network_thread.h"
#include "roomentry.h"
#include "session_level.pb.h"
#include "roomwidget.h"
#include "authorizationcredentials.h"

#include <QApplication>
#include <QSharedPointer>

class QNetworkDatagram;

class Application: public QApplication
{
    Q_OBJECT

public:
    Application (int& argc, char** argv);
    ~Application ();
    void start ();

signals:
    void roomListUpdated (const QVector<RoomEntry>& room_list);
    void queryReadiness ();
    void startCountdown (Unit::Team team);
    void startMatch ();
    void updateMatchState (const QVector<QPair<quint32, Unit>>& units, const QVector<QPair<quint32, Corpse>>& corpses, const QVector<QPair<quint32, Missile>>& missiles);

private:
    void showLobby (const QString& login);
    void showRoom (bool single_mode = false);
    void setCurrentWindow (QWidget* new_window);
    void startSingleMode (RoomWidget* room_widget);
    bool parseMatchState (const RTS::MatchStateResponse& response,
                          QVector<QPair<quint32, Unit>>& units, QVector<QPair<quint32, Corpse>>& corpses, QVector<QPair<quint32, Missile>>& missiles,
                          QString& error_message);
    std::optional<std::pair<quint32, Unit>> parseUnit (const RTS::Unit& r_unit, QString& error_message);
    std::optional<std::pair<quint32, Corpse>> parseCorpse (const RTS::Corpse& r_corpse, QString& error_message);
    std::optional<std::pair<quint32, Missile>> parseMissile (const RTS::Missile& r_missile, QString& error_message);
    QVector<AuthorizationCredentials> loadCredentials ();

private slots:
    void quitCallback ();
    void authorizationPromptCallback ();
    void loginCallback (const AuthorizationCredentials& credentials);
    void createRoomCallback (const QString& name);
    void joinRoomCallback (quint32 name);
    void sessionDatagramHandler (const QSharedPointer<HCCN::ServerToClient::Message>& message);
    void selectRolePlayerCallback ();
    void joinSpectatorCallback ();
    void readinessCallback ();
    void matchStartCallback ();
    void createUnitCallback (Unit::Team team, Unit::Type type, QPointF positon);
    void savedCredentials (const QVector<AuthorizationCredentials>& credentials);
    // void unitActionCallback (quint32 id, ActionType type, std::variant<QPointF, quint32> target);

    void unitActionCallback (quint32 id, const std::variant<StopAction, MoveAction, AttackAction, CastAction>& action);

private:
    void selectRolePlayer ();
    bool single_mode = false;
    QWidget* current_window = nullptr;
    QSharedPointer<NetworkThread> network_thread;
    QHostAddress host_address;
    quint16 port;
    QString login;
    std::optional<quint64> session_id;
    quint64 request_id = 0;
    quint32 last_tick = 0;
};
