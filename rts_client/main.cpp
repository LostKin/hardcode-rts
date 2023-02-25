#include "roomwidget.h"
#include "authorizationwidget.h"
#include "lobbywidget.h"
#include "application.h"

int main (int argc, char* argv[])
{
    Application a (argc, argv);

    a.start ();

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
    room.awaitTeamSelection (Unit::Team::Red);
    room.grabMouse ();
    room.grabKeyboard ();
    room.showFullScreen ();
#endif

#if 0
    RoomWidget room;
    room.queryReadiness (Unit::Team::Red);
    room.grabMouse ();
    room.grabKeyboard ();
    room.showFullScreen ();
#endif

#if 0
    RoomWidget room;
    room.ready (Unit::Team::Red);
    room.grabMouse ();
    room.grabKeyboard ();
    room.showFullScreen ();
#endif

#if 0
    RoomWidget room;
    room.awaitMatch (Unit::Team::Red);
    room.grabMouse ();
    room.grabKeyboard ();
    room.showFullScreen ();
#endif

#if 0
    RoomWidget room;
    room.grabMouse ();
    room.grabKeyboard ();
    room.showFullScreen ();
    room.startMatch (Unit::Team::Red);
#endif

    return a.exec ();
}
