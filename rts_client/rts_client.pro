QT += core gui widgets

CONFIG += c++11

SOURCES += \
    main.cpp \
    openglwidget.cpp

HEADERS += \
    openglwidget.h

RESOURCES += rts_client.qrc

MOC_DIR = .moc
OBJECTS_DIR = .obj
RCC_DIR = .qrc
