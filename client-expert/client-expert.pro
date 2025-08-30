TEMPLATE = app
TARGET = client-expert
QT += core gui widgets network
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

# Alternative modern UI target
modern {
    TARGET = client-expert-modern
    SOURCES -= src/main.cpp
    SOURCES += src/main_modern.cpp \
               src/modernmainwindow.cpp
    HEADERS += src/modernmainwindow.h
    RESOURCES += resources.qrc
}
