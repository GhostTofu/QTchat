QT       += core network sql widgets

TARGET    = ChatServer
TEMPLATE  = app

# 从Console改为GUI应用
CONFIG   -= console
CONFIG   -= app_bundle

# 修复MSVC编译器UTF-8中文编码问题
msvc {
    QMAKE_CXXFLAGS += /utf-8
}

SOURCES += main.cpp \
           chatserver.cpp \
           clienthandler.cpp \
           databasemanager.cpp \
           servermainwindow.cpp

HEADERS += chatserver.h \
           clienthandler.h \
           databasemanager.h \
           servermainwindow.h

FORMS   += servermainwindow.ui
