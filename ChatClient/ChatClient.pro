QT       += core gui network sql widgets multimedia

TARGET    = ChatClient
TEMPLATE  = app

# 修复MSVC编译器UTF-8中文编码问题
msvc {
    QMAKE_CXXFLAGS += /utf-8
}

SOURCES += main.cpp \
           loginwidget.cpp \
           mainwidget.cpp \
           chatdialog.cpp \
           networkclient.cpp \
           databasemanager.cpp \
           emojipicker.cpp \
           friendlistwidget.cpp \
           baiduspeech.cpp \
           audiorecorder.cpp

HEADERS += loginwidget.h \
           mainwidget.h \
           chatdialog.h \
           networkclient.h \
           databasemanager.h \
           emojipicker.h \
           friendlistwidget.h \
           baiduspeech.h \
           audiorecorder.h

FORMS   += loginwidget.ui \
           mainwidget.ui \
           chatdialog.ui \
           emojipicker.ui

RESOURCES += resources.qrc
