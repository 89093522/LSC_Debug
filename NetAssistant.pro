#-------------------------------------------------
#
# Project created by QtCreator 2015-06-27T22:18:28
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = lsc_tools-A
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++0x

SOURCES += main.cpp\
        mainwindow.cpp \
    udpclient.cpp \
    commonhelper.cpp \
    FtpManager.cpp

HEADERS  += mainwindow.h \
    udpclient.h \
    commonhelper.h \
    FtpManager.h

FORMS    += mainwindow.ui
RC_FILE = net.rc
