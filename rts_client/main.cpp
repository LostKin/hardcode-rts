#include "application.h"


int main (int argc, char* argv[])
{
    QApplication::setAttribute (Qt::AA_ShareOpenGLContexts, true);

    Application a (argc, argv);
    a.start ();

    return a.exec ();
}
