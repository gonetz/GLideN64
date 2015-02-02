#-------------------------------------------------
#
# Project created by QtCreator 2015-01-26T21:59:49
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GLideNUI
TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    ConfigDialog.cpp \
    GLideNUI.cpp \
    FullscreenResolutions_windows.cpp

HEADERS += \
    ConfigDialog.h \
    GLideNUI.h \
    FullscreenResolutions.h

RESOURCES +=

FORMS += \
    configDialog.ui
