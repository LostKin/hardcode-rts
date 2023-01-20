#pragma once

#include "roomentry.h"

#include <QWidget>

class RoomListTable;

class LobbyWidget: public QWidget
{
    Q_OBJECT

public:
    LobbyWidget (const QString& login, QWidget* parent = nullptr);
    ~LobbyWidget ();
    void setRoomList (const QList<RoomEntry>& room_list);

signals:
    void createRoomRequested ();
    void joinRoomRequested (uint32_t room_id);
    void logoutRequested ();

private slots:
    void joinRoomClicked ();
    void profileButtonClicked ();

private:
    QString login;
    RoomListTable* room_list_table;
};
