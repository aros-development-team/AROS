# ftinspect.pro

QMAKE_CXXFLAGS += -isystem ../../../freetype2/include

# To avoid conflicts with the FreeType version compiled into or used by Qt,
# we use the static library.
#
# You should adapt this to your setup.
unix|macx {
  LIBS += ../../../freetype2/objs/.libs/libfreetype.a

  CONFIG += link_pkgconfig
  PKGCONFIG += libpng harfbuzz zlib bzip2
}
win32 {
  LIBS += ../../../freetyp2/objs/vc2010/freetype2100.lib
  LIBS += -lpng -lharfbuzz -lz -lbz2 -lm
}

CONFIG += qt debug

# we need access to internal FreeType header files
DEFINES += FT2_BUILD_LIBRARY

SOURCES += \
  engine/engine.cpp \
  rendering/glyphbitmap.cpp \
  rendering/glyphoutline.cpp \
  rendering/glyphpointnumbers.cpp \
  rendering/glyphpoints.cpp \
  rendering/grid.cpp \
  widgets/qcomboboxx.cpp \
  widgets/qgraphicsviewx.cpp \
  widgets/qpushbuttonx.cpp \
  widgets/qspinboxx.cpp \
  ftinspect.cpp \
  maingui.cpp

HEADERS += \
  engine/engine.hpp \
  rendering/glyphbitmap.hpp \
  rendering/glyphoutline.hpp \
  rendering/glyphpointnumbers.hpp \
  rendering/glyphpoints.hpp \
  rendering/grid.hpp \
  widgets/qcomboboxx.hpp \
  widgets/qgraphicsviewx.hpp \
  widgets/qpushbuttonx.hpp \
  widgets/qspinboxx.hpp \
  maingui.hpp

TARGET = ftinspect

QT += widgets


# end of ftinpect.pro
