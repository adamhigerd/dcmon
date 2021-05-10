TEMPLATE = app
TARGET = dcmon
CONFIG += link_pkgconfig debug
PKGCONFIG += lua53-c++
QT = core widgets

HEADERS = dclog.h   dcps.h   dclogview.h   dctoolbar.h   treelogmodel.h
SOURCES = dclog.cpp dcps.cpp dclogview.cpp dctoolbar.cpp treelogmodel.cpp main.cpp
