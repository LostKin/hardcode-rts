QT += core network

CONFIG += c++11 debug

SOURCES += \
    main.cpp \
    application.cpp \
    network_thread.cpp \
    network_manager.cpp

HEADERS += \
    application.h \
    network_thread.h \
    network_manager.h

LIBS += -lprotobuf

MOC_DIR = .moc
OBJECTS_DIR = .obj

PROTOC_SRC = ../api/entities.proto

protoc.output = .proto_stubs/${QMAKE_FILE_BASE}.pb.cc
protoc.commands = protoc ${QMAKE_FILE_NAME} -I../api --cpp_out=.proto_stubs
protoc.input = PROTOC_SRC

QMAKE_EXTRA_COMPILERS += protoc
