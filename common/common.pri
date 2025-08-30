INCLUDEPATH += $$PWD
SOURCES += $$PWD/protocol.cpp \
           $$PWD/auth/authwidget.cpp
HEADERS += $$PWD/protocol.h \
           $$PWD/auth/authwidget.h

# Add widgets module for auth components
QT += widgets
