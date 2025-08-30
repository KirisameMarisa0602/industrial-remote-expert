TEMPLATE = app
TARGET = server
QT += core network sql
CONFIG += c++11 console
CONFIG -= app_bundle
SOURCES += src/main.cpp \
           src/roomhub.cpp \
           ../common/protocol.cpp
HEADERS += src/roomhub.h \
           ../common/protocol.h
INCLUDEPATH += ../common
RESOURCES += ../common/resources.qrc
