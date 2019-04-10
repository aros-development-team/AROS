// ftinspect.cpp

// Copyright (C) 2016-2019 by Werner Lemberg.


#include "maingui.hpp"
#include "engine/engine.hpp"

#include <QApplication>

#define VERSION "X.Y.Z"


int
main(int argc,
     char** argv)
{
  QApplication app(argc, argv);
  app.setApplicationName("ftinspect");
  app.setApplicationVersion(VERSION);
  app.setOrganizationName("FreeType");
  app.setOrganizationDomain("freetype.org");

  MainGUI gui;
  Engine engine(&gui);

  gui.update(&engine);
  gui.setDefaults();

  gui.show();

  return app.exec();
}


// end of ftinspect.cpp
