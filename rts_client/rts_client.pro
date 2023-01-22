QT += core gui widgets network

CONFIG += c++17
CONFIG += qt
CONFIG += debug_and_release

include (../match_state/match_state.pri)

SOURCES += \
    main.cpp \
    network_thread.cpp \
    network_manager.cpp \
    application.cpp \
    openglwidget.cpp \
    authorizationwidget.cpp \
    authorizationprogresswidget.cpp \
    lobbywidget.cpp \
    roomlisttable.cpp \
    roomsettingswidget.cpp \
    profilewidget.cpp \
    roomwidget.cpp \
    roomentry.cpp

HEADERS += \
    network_thread.h \
    network_manager.h \
    application.h \
    openglwidget.h \
    authorizationwidget.h \
    authorizationprogresswidget.h \
    lobbywidget.h \
    roomlisttable.h \
    roomsettingswidget.h \
    profilewidget.h \
    roomwidget.h \
    roomentry.h

INCLUDEPATH += .proto_stubs
LIBS += -lprotobuf

RESOURCES += rts_client.qrc

MOC_DIR = .moc
OBJECTS_DIR = .obj
RCC_DIR = .qrc

PROTOC_SRC = ../api/session_level.proto

protoc.output = .proto_stubs/${QMAKE_FILE_BASE}.pb.cc
protoc.commands = protoc ${QMAKE_FILE_NAME} -I../api --cpp_out=.proto_stubs
protoc.input = PROTOC_SRC

QMAKE_EXTRA_COMPILERS += protoc
