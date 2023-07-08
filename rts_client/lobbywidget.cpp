#include "lobbywidget.h"

#include "roomlisttable.h"
#include "roomsettingswidget.h"
#include "profilewidget.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDebug>

LobbyWidget::LobbyWidget (const QString& login, QWidget* parent)
    : QWidget (parent)
    , login (login)
{
    QVBoxLayout* layout = new QVBoxLayout;
    {
        QHBoxLayout* hbox = new QHBoxLayout;
        {
            QPushButton* button = new QPushButton ("Create room...", this);
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
    setLayout (layout);
}
LobbyWidget::~LobbyWidget ()
{
}

void LobbyWidget::setRoomList (const QVector<RoomEntry>& room_list)
{
    room_list_table->setRoomList (room_list);
    room_list_loaded = true;
}
void LobbyWidget::joinRoomClicked ()
{
    QVariant current_room = room_list_table->getCurrentRoom ();
    if (current_room.type () == QVariant::UInt)
        emit joinRoomRequested (current_room.toUInt ());
}
void LobbyWidget::createRoomButtonClicked ()
{
    QWidget* button = (QWidget*) sender ();
    RoomSettingsWidget popup (button);
    connect (&popup, SIGNAL (createRequested (const QString&)), this, SIGNAL (createRoomRequested (const QString&)));
    QPoint left_bottom = button->mapToGlobal (QPoint (0, 0)) + QPoint (0, button->height ());
    popup.show ();
    popup.move (left_bottom.x (), left_bottom.y () + 4);
    popup.exec ();
}
void LobbyWidget::profileButtonClicked ()
{
    QWidget* button = (QWidget*) sender ();
    ProfileWidget popup (login, button);
    connect (&popup, SIGNAL (logoutRequested ()), this, SIGNAL (logoutRequested ()));
    QPoint right_bottom = button->mapToGlobal (QPoint (0, 0)) + QPoint (button->width (), button->height ());
    popup.show ();
    popup.move (right_bottom.x () - popup.width (), right_bottom.y () + 4);
    popup.exec ();
}
