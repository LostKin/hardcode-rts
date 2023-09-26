QT += core network

CONFIG += c++17
CONFIG += qt
CONFIG += debug_and_release

include (../match_state/match_state.pri)
include (../libhccn/libhccn.pri)

SOURCES += \
    main.cpp \
    application.cpp \
    network_thread.cpp \
    network_manager.cpp \
    room_thread.cpp \
    room.cpp \

HEADERS += \
    application.h \
    network_thread.h \
    network_manager.h \
    room_thread.h \
    room.h \

INCLUDEPATH += .proto_stubs
LIBS += -lprotobuf

MOC_DIR = .moc
OBJECTS_DIR = .obj

PROTOC_SRC = ../api/session_level.proto

protoc.output = .proto_stubs/${QMAKE_FILE_BASE}.pb.cc
protoc.commands = protoc ${QMAKE_FILE_NAME} -I../api --cpp_out=.proto_stubs
protoc.input = PROTOC_SRC

QMAKE_EXTRA_COMPILERS += protoc
