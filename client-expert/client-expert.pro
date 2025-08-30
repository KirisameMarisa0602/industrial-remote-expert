TEMPLATE = app
TARGET = client-expert
QT += core gui widgets network
CONFIG += c++11
SOURCES += src/main.cpp \
           src/mainwindow.cpp \
           src/clientconn.cpp \
           src/loginwindow.cpp \
           src/expertmain.cpp \
           src/factorymain.cpp
HEADERS += src/mainwindow.h \
           src/clientconn.h \
           src/loginwindow.h \
           src/expertmain.h \
           src/factorymain.h
FORMS   +=
include(../common/common.pri)
QT += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
