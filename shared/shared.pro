TEMPLATE = lib
CONFIG += staticlib c++11
TARGET = shared

QT += core gui widgets

INCLUDEPATH += $$PWD

SOURCES += \
    src/loginregisterdialog.cpp \
    src/modernstyle.cpp

HEADERS += \
    src/loginregisterdialog.h \
    src/modernstyle.h

RESOURCES += \
    resources/shared.qrc