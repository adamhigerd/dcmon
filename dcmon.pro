TEMPLATE = app
TARGET = dcmon
CONFIG += link_pkgconfig debug
PKGCONFIG += lua53-c++
QT = core widgets
MOC_DIR = .obj
OBJECTS_DIR = .obj

HEADERS += src/dclog.h   src/dcps.h   src/dclogview.h   src/dctoolbar.h   src/treelogmodel.h
SOURCES += src/dclog.cpp src/dcps.cpp src/dclogview.cpp src/dctoolbar.cpp src/treelogmodel.cpp

HEADERS += src/luavm.h   src/luatable.h   src/luafunction.h
SOURCES += src/luavm.cpp src/luatable.cpp src/luafunction.cpp src/main.cpp
