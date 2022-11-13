QT += core network

CONFIG += c++11

SOURCES += \
    main.cpp \
    application.cpp \
    network_thread.cpp \
    network_manager.cpp

HEADERS += \
    application.h \
    network_thread.h \
    network_manager.h

MOC_DIR = .moc
OBJECTS_DIR = .obj
