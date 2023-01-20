#include "roomwidget.h"
#include "authorizationwidget.h"
#include "lobbywidget.h"

#include <QApplication>


int main (int argc, char* argv[])
{
    QApplication a (argc, argv);

#if 0
    AuthorizationWidget w1;
    w1.show ();
#endif

#if 0
    AuthorizationWidget w1;
    LobbyWidget w2 ("john-smith");
    w2.setRoomList ({
        {30, "hi"},
        {35, "john"},
    });
    w2.showMaximized ();
#endif

#if 0
    RoomWidget room;
    room.grabMouse ();
    room.grabKeyboard ();
    room.showFullScreen ();
#endif

#if 0
    RoomWidget room;
    room.awaitTeamSelection (RoomWidget::Team::Red);
    room.grabMouse ();
    room.grabKeyboard ();
    room.showFullScreen ();
#endif

#if 0
    RoomWidget room;
    room.queryReadiness (RoomWidget::Team::Red);
    room.grabMouse ();
    room.grabKeyboard ();
    room.showFullScreen ();
#endif

#if 0
    RoomWidget room;
    room.ready (RoomWidget::Team::Red);
    room.grabMouse ();
    room.grabKeyboard ();
    room.showFullScreen ();
#endif

#if 0
    RoomWidget room;
    room.awaitMatch (RoomWidget::Team::Red);
    room.grabMouse ();
    room.grabKeyboard ();
    room.showFullScreen ();
#endif

#if 1
    RoomWidget room;
    room.grabMouse ();
    room.grabKeyboard ();
    room.showFullScreen ();
    room.startMatch (RoomWidget::Team::Red);
#endif

    return a.exec ();
}
