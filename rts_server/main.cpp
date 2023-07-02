#include "application.h"

int main (int argc, char** argv)
{
    Application app (argc, argv);

    if (!app.init ()) {
        qDebug () << "Failed to init";
        return 1;
    }
    return app.exec ();
}
