QT += core gui widgets

CONFIG += c++17
CONFIG += qt
CONFIG += debug_and_release

SOURCES += \
    main.cpp \
    openglwidget.cpp \
    authorizationwidget.cpp \
    lobbywidget.cpp \
    roomlisttable.cpp \
    profilewidget.cpp \
    roomwidget.cpp \
    matchstate.cpp \
    roomentry.cpp

HEADERS += \
    openglwidget.h \
    authorizationwidget.h \
    lobbywidget.h \
    roomlisttable.h \
    profilewidget.h \
    roomwidget.h \
    matchstate.h \
    roomentry.h

RESOURCES += rts_client.qrc

MOC_DIR = .moc
OBJECTS_DIR = .obj
RCC_DIR = .qrc
