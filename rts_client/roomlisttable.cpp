#include "roomlisttable.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QHeaderView>


RoomListTable::RoomListTable (QWidget* parent)
    : QTableWidget (0, 7, parent)
{
    setHorizontalHeaderLabels ({"ID", "Name", "Clients", "Players", "Ready", "Spectators", ""});
    horizontalHeader ()->setStretchLastSection (true);
    horizontalHeader ()->setSectionResizeMode (2, QHeaderView::ResizeToContents);
    horizontalHeader ()->setSectionResizeMode (3, QHeaderView::ResizeToContents);
    horizontalHeader ()->setSectionResizeMode (4, QHeaderView::ResizeToContents);
    horizontalHeader ()->setSectionResizeMode (5, QHeaderView::ResizeToContents);
    horizontalHeader ()->setSectionResizeMode (6, QHeaderView::ResizeToContents);
    setEditTriggers (QAbstractItemView::NoEditTriggers);
    setSelectionBehavior (QAbstractItemView::SelectRows);
    connect (this, SIGNAL (cellClicked (int, int)), this, SLOT (cellClicked (int, int)));
    connect (this, SIGNAL (cellDoubleClicked (int, int)), this, SLOT (cellDoubleClicked (int, int)));
}
RoomListTable::~RoomListTable ()
{
}

void RoomListTable::setRoomList (const QVector<RoomEntry>& room_list)
{
    setRowCount (room_list.size ());
    for (size_t row = 0; row < size_t (room_list.size ()); ++row) {
        const RoomEntry& entry = room_list[row];
        {
            QTableWidgetItem* item = new QTableWidgetItem (tr ("%1").arg (entry.id));
            setItem (row, 0, item);
        }
        {
            QTableWidgetItem* item = new QTableWidgetItem (entry.name);
            item->setData (Qt::UserRole, uint (entry.id));
            setItem (row, 1, item);
        }
        {
            QTableWidgetItem* item = new QTableWidgetItem (tr ("%1").arg (entry.client_count));
            setItem (row, 2, item);
        }
        {
            QTableWidgetItem* item = new QTableWidgetItem (tr ("%1").arg (entry.player_count));
            setItem (row, 3, item);
        }
        {
            QTableWidgetItem* item = new QTableWidgetItem (tr ("%1").arg (entry.ready_player_count));
            setItem (row, 4, item);
        }
        {
            QTableWidgetItem* item = new QTableWidgetItem (tr ("%1").arg (entry.spectator_count));
            setItem (row, 5, item);
        }
        {
            QTableWidgetItem* item = new QTableWidgetItem;
            QIcon icon (":/images/icons/delete.png");
            item->setIcon (icon);
            setItem (row, 6, item);
        }
    }
}
QVariant RoomListTable::getCurrentRoom ()
{
    int row = currentRow ();
    QTableWidgetItem* name_item = item (row, 1);
    if (!name_item)
        return {};
    return name_item->data (Qt::UserRole);
}
void RoomListTable::cellClicked (int row, int column)
{
    QTableWidgetItem* name_item = item (row, 1);
    if (!name_item)
        return;
    uint32_t room_id = name_item->data (Qt::UserRole).toUInt ();
    if (column == 6) {
        QString room_name = name_item->data (Qt::DisplayRole).toString ();
        QMessageBox::StandardButton reply = QMessageBox::question (this, "Delete room?",
                                                                   "Do you really want to delete room '" + room_name + "'?", QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes)
            emit deleteRoomRequested (room_id);
    }
}
void RoomListTable::cellDoubleClicked (int row, int column)
{
    QTableWidgetItem* name_item = item (row, 1);
    if (!name_item)
        return;
    uint32_t room_id = name_item->data (Qt::UserRole).toUInt ();
    if (column >= 0 && column <= 5)
        emit joinRoomRequested (room_id);
}
