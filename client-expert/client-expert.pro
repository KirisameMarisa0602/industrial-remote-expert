TEMPLATE = app
TARGET = client-expert
QT += core gui widgets network multimedia multimediawidgets charts
CONFIG += c++11

SOURCES += src/main.cpp \
           src/mainwindow.cpp \
           src/clientconn.cpp

HEADERS += src/mainwindow.h \
           src/clientconn.h

# Link with static libraries
LIBS += -L$$OUT_PWD/../shared -lshared
LIBS += -L$$OUT_PWD/../common -lcommon

# Include paths for dependencies
INCLUDEPATH += $$PWD/../shared/src
INCLUDEPATH += $$PWD/../common

# Dependencies
PRE_TARGETDEPS += $$OUT_PWD/../shared/libshared.a
PRE_TARGETDEPS += $$OUT_PWD/../common/libcommon.a

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
