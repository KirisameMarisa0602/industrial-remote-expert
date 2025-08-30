TEMPLATE = app
TARGET = launcher
QT += core gui widgets network
CONFIG += c++11

SOURCES += src/main.cpp \
           src/mainwindow.cpp \
           src/clientconn.cpp

HEADERS += src/mainwindow.h \
           src/clientconn.h

include(../common/common.pri)

RESOURCES += resources.qrc