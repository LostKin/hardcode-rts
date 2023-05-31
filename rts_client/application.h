#pragma once

#include "network_thread.h"
#include "roomentry.h"
#include "session_level.pb.h"
#include "roomwidget.h"

#include <QApplication>
#include <QSharedPointer>

class QNetworkDatagram;
//class MatchState;
//class Unit;

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
    void startCountdown();
    void startMatch ();
    void updateMatchState (QVector<QPair<quint32, Unit> > units, QVector<QPair<quint32, Missile> > missiles);

private:
    void showLobby (const QString& login);
    void showRoom (bool single_mode = false);
    void setCurrentWindow (QWidget* new_window);
    void startSingleMode (RoomWidget* room_widget);

private slots:
    void quitCallback ();
    void authorizationPromptCallback ();
    void loginCallback (const QString& host, quint16 port, const QString& login, const QString& password);
    void createRoomCallback (const QString& name);
    void joinRoomCallback (quint32 name);
    void sessionDatagramHandler (QSharedPointer<QNetworkDatagram> datagram);
    void joinRedTeamCallback ();
    void joinBlueTeamCallback ();
    void joinSpectatorCallback ();
    void readinessCallback ();
    void matchStartCallback ();
    void createUnitCallback (Unit::Team team, Unit::Type type, QPointF positon);
    //void unitActionCallback (quint32 id, ActionType type, std::variant<QPointF, quint32> target);

    void unitActionCallback (quint32 id, const std::variant<MoveAction, AttackAction, CastAction>& action);


private:
    void joinTeam(RTS::Team id);
    bool single_mode = false;
    QWidget* current_window = nullptr;
    QSharedPointer<NetworkThread> network_thread;
    QHostAddress host_address;
    quint16 port;
    QString login;
    std::optional<quint64> session_token;
    quint64 request_token = 0;
};
