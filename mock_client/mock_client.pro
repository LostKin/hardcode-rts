QT += core widgets network

CONFIG += c++11

SOURCES += \
    main.cpp

LIBS += -lprotobuf

MOC_DIR = .moc
OBJECTS_DIR = .obj

PROTOC_SRC = ../api/session_level.proto

protoc.output = .proto_stubs/${QMAKE_FILE_BASE}.pb.cc
protoc.commands = protoc ${QMAKE_FILE_NAME} -I../api --cpp_out=.proto_stubs
protoc.input = PROTOC_SRC

QMAKE_EXTRA_COMPILERS += protoc