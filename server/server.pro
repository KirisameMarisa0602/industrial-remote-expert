TEMPLATE = app
TARGET = server
QT += core network sql serialport
CONFIG += c++11 console
CONFIG -= app_bundle
SOURCES += src/main.cpp \
           src/roomhub.cpp \
           src/database.cpp
HEADERS += src/roomhub.h \
           src/database.h
include(../common/common.pri)
