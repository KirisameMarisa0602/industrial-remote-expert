TEMPLATE = lib
CONFIG += staticlib c++11
TARGET = common

QT += core network

INCLUDEPATH += $$PWD

SOURCES += \
    protocol.cpp

HEADERS += \
    protocol.h