INCLUDEPATH += $$PWD
SOURCES += $$PWD/protocol.cpp
HEADERS += $$PWD/protocol.h

# UI components (only for GUI applications)
contains(QT, widgets) {
    INCLUDEPATH += $$PWD/ui
    SOURCES += $$PWD/ui/loginregisterdialog.cpp
    HEADERS += $$PWD/ui/loginregisterdialog.h
    RESOURCES += $$PWD/resources/resources.qrc
}
