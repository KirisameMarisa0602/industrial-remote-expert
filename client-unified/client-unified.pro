TEMPLATE = app
TARGET = client-unified

QT += quick qml quickcontrols2 multimedia websockets sql

CONFIG += c++11

SOURCES += \
    src/main.cpp \
    src/video/FrameGrabberFilter.cpp

HEADERS += \
    src/video/FrameGrabberFilter.h

RESOURCES += qml/qml.qrc

QMAKE_CXXFLAGS += -std=c++11
