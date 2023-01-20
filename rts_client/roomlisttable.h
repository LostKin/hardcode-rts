#pragma once

#include "roomentry.h"

#include <QTableWidget>

class RoomListTable: public QTableWidget
{
    Q_OBJECT

public:
    RoomListTable (QWidget* parent = nullptr);
    ~RoomListTable ();
    void setRoomList (const QList<RoomEntry>& room_list);
    QVariant getCurrentRoom ();

signals:
    void joinRoomRequested (uint32_t room_id);
    void deleteRoomRequested (uint32_t room_id);

private slots:
    void cellClicked (int row, int column);
    void cellDoubleClicked (int row, int column);
};
