TEMPLATE = app
TARGET = client-factory
QT += core gui widgets network serialport charts
CONFIG += c++11
SOURCES += src/main.cpp \
           src/mainwindow.cpp \
           src/clientconn.cpp
HEADERS += src/mainwindow.h \
           src/clientconn.h
FORMS   +=
include(../common/common.pri)
QT += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
