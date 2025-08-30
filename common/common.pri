INCLUDEPATH += $$PWD
SOURCES += $$PWD/protocol.cpp \
           $$PWD/auth/authwidget.cpp \
           $$PWD/dashboard/dashboardwidget.cpp \
           $$PWD/sidebar/sidebarwidget.cpp
HEADERS += $$PWD/protocol.h \
           $$PWD/auth/authwidget.h \
           $$PWD/dashboard/dashboardwidget.h \
           $$PWD/sidebar/sidebarwidget.h

# Add widgets module for auth components
QT += widgets
