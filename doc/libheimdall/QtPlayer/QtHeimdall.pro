TEMPLATE = app
TARGET = qtheimdall

CONFIG += link_pkgconfig force_debug_info
PKGCONFIG = libheimdall
QT += widgets

HEADERS += player.h
SOURCES += main.cpp player.cpp
