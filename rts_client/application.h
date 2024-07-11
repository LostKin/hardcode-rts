#pragma once

#include "network_thread.h"
#include "roomentry.h"
#include "responses.pb.h"
#include "roomwidget.h"
#include "authorizationcredentials.h"

#include <QApplication>
#include <QSharedPointer>

class QNetworkDatagram;
class MainWindow;

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
    void updateMatchState (const std::vector<std::pair<quint32, Unit>>& units, const std::vector<std::pair<quint32, Corpse>>& corpses, const std::vector<std::pair<quint32, Missile>>& missiles);
    void log (const QString& message);

private:
    void showLobby (const QString& login);
    void showRoleSelectionScreen ();
    void showRoleSelectionProgressScreen ();
    void showReadinessScreen ();
    void showRoom (bool single_mode = false);
    void setCurrentWindow (QWidget* new_window, bool fullscreen = false);
    void startSingleMode (RoomWidget* room_widget);
    QVector<AuthorizationCredentials> loadCredentials ();

private slots:
    void quitCallback ();
    void authorizationPromptCallback ();
    void loginCallback (const AuthorizationCredentials& credentials);
    void createRoomCallback (const QString& name);
    void joinRoomCallback (quint32 name);
    void roleSelectedCallback ();
    void sessionDatagramHandler (const std::shared_ptr<HCCN::ServerToClient::Message>& message);
    void selectRolePlayerCallback ();
    void joinSpectatorCallback ();
    void readinessCallback ();
    void matchStartCallback ();
    void createUnitCallback (Unit::Team team, Unit::Type type, const Position& positon);
    void savedCredentials (const QVector<AuthorizationCredentials>& credentials);
    void unitActionCallback (quint32 id, const UnitActionVariant& action);

private:
    void selectRolePlayer ();
    bool single_mode = false;
    MainWindow* main_window = nullptr;
    QSharedPointer<NetworkThread> network_thread;
    QHostAddress host_address;
    quint16 port;
    QString login;
    std::optional<quint64> session_id;
    quint64 request_id = 0;
    quint32 last_tick = 0;
};
