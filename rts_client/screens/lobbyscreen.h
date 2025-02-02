#pragma once

#include "roomentry.h"

#include <QWidget>


class RoomListTable;


class LobbyScreen: public QWidget
{
    Q_OBJECT

public:
    LobbyScreen (const QString& login, QWidget* parent = nullptr);
    ~LobbyScreen ();

public slots:
    void setRoomList (const QVector<RoomEntry>& room_list);

signals:
    void createRoomRequested (const QString& room_name);
    void joinRoomRequested (quint32 room_id);
    void logoutRequested ();

private slots:
    void joinRoomClicked ();
    void createRoomButtonClicked ();
    void profileButtonClicked ();

private:
    QString login;
    bool room_list_loaded = false;
    RoomListTable* room_list_table;
};
