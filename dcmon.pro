TEMPLATE = app
TARGET = dcmon
QT = core widgets
MOC_DIR = .obj
OBJECTS_DIR = .obj

!isEmpty(DEBUG) {
  CONFIG += debug
}

HEADERS += src/dclog.h   src/dcps.h   src/dclogview.h   src/dclogtab.h   src/dctoolbar.h   src/treelogmodel.h
SOURCES += src/dclog.cpp src/dcps.cpp src/dclogview.cpp src/dclogtab.cpp src/dctoolbar.cpp src/treelogmodel.cpp

HEADERS += src/dcmonconfig.h   src/fileutil.h
SOURCES += src/dcmonconfig.cpp src/fileutil.cpp src/main.cpp

!isEmpty(USE_LUA) {
  CONFIG += link_pkgconfig
  PKGCONFIG += lua53-c++
  DEFINES += D_USE_LUA=1

  HEADERS += src/luavm.h   src/luatable.h   src/luafunction.h
  SOURCES += src/luavm.cpp src/luatable.cpp src/luafunction.cpp
}
