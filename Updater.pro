QT += core network
QT -= gui

TARGET = Updater
CONFIG += console c++11
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

DISTFILES += \
    Updater.ini

