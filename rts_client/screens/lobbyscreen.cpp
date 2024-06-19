#include "lobbyscreen.h"

#include "roomlisttable.h"
#include "roomsettingspopup.h"
#include "profilepopup.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>


LobbyScreen::LobbyScreen (const QString& login, QWidget* parent)
    : QWidget (parent)
    , login (login)
{
    QGridLayout* decoration_grid = new QGridLayout;
    decoration_grid->setRowStretch (0, 1);
    decoration_grid->setRowStretch (2, 4);
    decoration_grid->setRowStretch (3, 1);
    decoration_grid->setColumnStretch (0, 1);
    decoration_grid->setColumnStretch (1, 4);
    decoration_grid->setColumnStretch (2, 1);
    {
        QLabel* label = new QLabel ("<b>Room list</b>", this);
        decoration_grid->addWidget (label, 1, 1, Qt::AlignHCenter);
    }
    {
        QVBoxLayout* layout = new QVBoxLayout;
        {
            QHBoxLayout* hbox = new QHBoxLayout;
            {
                QPushButton* button = new QPushButton ("&Create room...", this);
                connect (button, SIGNAL (clicked ()), this, SLOT (createRoomButtonClicked ()));
                hbox->addWidget (button);
            }
            hbox->addStretch (1);
            {
                QIcon icon (":/images/icons/user.png");
                QPushButton* button = new QPushButton (icon, "", this);
                connect (button, SIGNAL (clicked ()), this, SLOT (profileButtonClicked ()));
                hbox->addWidget (button);
            }
            layout->addLayout (hbox);
        }
        {
            QHBoxLayout* hbox = new QHBoxLayout;
            {
                QLabel* label = new QLabel ("Current rooms:", this);
                hbox->addWidget (label);
            }
            hbox->addStretch (1);
            layout->addLayout (hbox);
        }
        {
            room_list_table = new RoomListTable (this);
            connect (room_list_table, &RoomListTable::joinRoomRequested, this, &LobbyScreen::joinRoomRequested);
            layout->addWidget (room_list_table, 1);
        }
        {
            QHBoxLayout* hbox = new QHBoxLayout;
            hbox->addStretch (1);
            {
                QPushButton* button = new QPushButton ("&Join", this);
                connect (button, SIGNAL (clicked ()), this, SLOT (joinRoomClicked ()));
                hbox->addWidget (button);
            }
            layout->addLayout (hbox);
        }
        decoration_grid->addLayout (layout, 2, 1);
    }
    setLayout (decoration_grid);
}
LobbyScreen::~LobbyScreen ()
{
}

void LobbyScreen::setRoomList (const QVector<RoomEntry>& room_list)
{
    room_list_table->setRoomList (room_list);
    room_list_loaded = true;
}
void LobbyScreen::joinRoomClicked ()
{
    QVariant current_room = room_list_table->getCurrentRoom ();
    if (current_room.typeId () == QVariant::UInt)
        emit joinRoomRequested (current_room.toUInt ());
}
void LobbyScreen::createRoomButtonClicked ()
{
    QWidget* button = (QWidget*) sender ();
    if (!button)
        return;
    RoomSettingsPopup popup (button);
    connect (&popup, SIGNAL (createRequested (const QString&)), this, SIGNAL (createRoomRequested (const QString&)));
    QPoint left_bottom = button->mapToGlobal (QPoint (0, 0)) + QPoint (0, button->height ());
    popup.show ();
    popup.move (left_bottom.x (), left_bottom.y () + 4);
    popup.exec ();
}
void LobbyScreen::profileButtonClicked ()
{
    QWidget* button = (QWidget*) sender ();
    if (!button)
        return;
    ProfilePopup popup (login, button);
    connect (&popup, SIGNAL (logoutRequested ()), this, SIGNAL (logoutRequested ()));
    QPoint right_bottom = button->mapToGlobal (QPoint (0, 0)) + QPoint (button->width (), button->height ());
    popup.show ();
    popup.move (right_bottom.x () - popup.width (), right_bottom.y () + 4);
    popup.exec ();
}
