TEMPLATE = app
TARGET = dcmon
### TODO: Lua-based configuration / filtering
# CONFIG += link_pkgconfig
# PKGCONFIG += lua53-c++
QT = core widgets
MOC_DIR = .obj
OBJECTS_DIR = .obj

HEADERS = src/dclog.h   src/dcps.h   src/dclogview.h   src/dctoolbar.h   src/treelogmodel.h
SOURCES = src/dclog.cpp src/dcps.cpp src/dclogview.cpp src/dctoolbar.cpp src/treelogmodel.cpp src/main.cpp
