TEMPLATE = app
TARGET = client-expert
QT += core gui widgets network multimedia multimediawidgets sql charts svg
CONFIG += c++11
SOURCES += src/main.cpp \
           src/mainwindow.cpp \
           src/clientconn.cpp \
           src/expertmainwindow.cpp
HEADERS += src/mainwindow.h \
           src/clientconn.h \
           src/expertmainwindow.h
FORMS   +=
include(../common/common.pri)

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
