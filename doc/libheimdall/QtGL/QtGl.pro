TEMPLATE = app
TARGET = qtglheimdall

CONFIG += c++14 link_pkgconfig force_debug_info
PKGCONFIG = libheimdall
QT += widgets
greaterThan(QT_MAJOR_VERSION, 5): QT += openglwidgets

SOURCES += main.cpp qtheimdallwidget.cpp
HEADERS += qtheimdallwidget.h
