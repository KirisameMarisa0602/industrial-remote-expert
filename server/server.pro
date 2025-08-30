TEMPLATE = app
TARGET = server
QT += core network sql
CONFIG += c++11 console
CONFIG -= app_bundle

SOURCES += src/main.cpp \
           src/roomhub.cpp

HEADERS += src/roomhub.h

# Link with static libraries
LIBS += -L$$OUT_PWD/../common -lcommon

# Include paths for dependencies
INCLUDEPATH += $$PWD/../common

# Dependencies
PRE_TARGETDEPS += $$OUT_PWD/../common/libcommon.a
