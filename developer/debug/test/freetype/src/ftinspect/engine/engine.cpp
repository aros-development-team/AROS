// engine.cpp

// Copyright (C) 2016-2017 by Werner Lemberg.


#include "engine.hpp"
#include "../maingui.hpp"

#include <stdexcept>
#include <stdint.h>

#include FT_DRIVER_H
#include FT_LCD_FILTER_H

// internal FreeType header files; only available in the source code bundle
#include FT_INTERNAL_DRIVER_H
#include FT_INTERNAL_OBJECTS_H


/////////////////////////////////////////////////////////////////////////////
//
// FaceID
//
/////////////////////////////////////////////////////////////////////////////

FaceID::FaceID()
: fontIndex(-1),
  faceIndex(-1),
  namedInstanceIndex(-1)
{
  // empty
}


FaceID::FaceID(int fontIdx,
               long faceIdx,
               int namedInstanceIdx)
: fontIndex(fontIdx),
  faceIndex(faceIdx),
  namedInstanceIndex(namedInstanceIdx)
{
  // empty
}


bool
FaceID::operator<(const FaceID& other) const
{
  bool ret = false;

  if (fontIndex < other.fontIndex)
    ret = true;
  else if (fontIndex == other.fontIndex)
  {
    if (faceIndex < other.faceIndex)
      ret = true;
    else if (faceIndex == other.faceIndex)
    {
      if (namedInstanceIndex < other.namedInstanceIndex)
        ret = true;
    }
  }

  return ret;
}


// The face requester is a function provided by the client application to
// the cache manager to translate an `abstract' face ID into a real
// `FT_Face' object.
//
// We use a map: `faceID' is the value, and its associated key gives the
// font, face, and named instance indices.  Getting a key from a value is
// slow, but this must be done only once, since `faceRequester' is only
// called if the font is not yet in the cache.

FT_Error
faceRequester(FTC_FaceID ftcFaceID,
              FT_Library library,
              FT_Pointer requestData,
              FT_Face* faceP)
{
  MainGUI* gui = static_cast<MainGUI*>(requestData);
  // `ftcFaceID' is actually an integer
  // -> first convert pointer to same-width integer, then discard superfluous
  //    bits (e.g., on x86_64 where pointers are wider than int)
  int val = static_cast<int>(reinterpret_cast<intptr_t>(ftcFaceID));
  // make sure this does not cause information loss
  Q_ASSERT_X(sizeof(void*) >= sizeof(int),
             "faceRequester",
             "Pointer size must be at least the size of int"
             " in order to treat FTC_FaceID correctly");

  const FaceID& faceID = gui->engine->faceIDMap.key(val);

  // this is the only place where we have to check the validity of the font
  // index; note that the validity of both the face and named instance index
  // is checked by FreeType itself
  if (faceID.fontIndex < 0
      || faceID.fontIndex >= gui->fontList.size())
    return FT_Err_Invalid_Argument;

  QString& font = gui->fontList[faceID.fontIndex];
  long faceIndex = faceID.faceIndex;

  if (faceID.namedInstanceIndex > 0)
    faceIndex += faceID.namedInstanceIndex << 16;

  return FT_New_Face(library,
                     qPrintable(font),
                     faceIndex,
                     faceP);
}


/////////////////////////////////////////////////////////////////////////////
//
// Engine
//
/////////////////////////////////////////////////////////////////////////////

Engine::Engine(MainGUI* g)
{
  gui = g;
  ftSize = NULL;
  // we reserve value 0 for the `invalid face ID'
  faceCounter = 1;

  FT_Error error;

  error = FT_Init_FreeType(&library);
  if (error)
  {
    // XXX error handling
  }

  error = FTC_Manager_New(library, 0, 0, 0,
                          faceRequester, gui, &cacheManager);
  if (error)
  {
    // XXX error handling
  }

  error = FTC_SBitCache_New(cacheManager, &sbitsCache);
  if (error)
  {
    // XXX error handling
  }

  error = FTC_ImageCache_New(cacheManager, &imageCache);
  if (error)
  {
    // XXX error handling
  }

  // query engines and check for alternatives

  // CFF
  error = FT_Property_Get(library,
                          "cff",
                          "hinting-engine",
                          &cffHintingEngineDefault);
  if (error)
  {
    // no CFF engine
    cffHintingEngineDefault = -1;
    cffHintingEngineOther = -1;
  }
  else
  {
    int engines[] =
    {
      FT_HINTING_FREETYPE,
      FT_HINTING_ADOBE
    };

    int i;
    for (i = 0; i < 2; i++)
      if (cffHintingEngineDefault == engines[i])
        break;

    cffHintingEngineOther = engines[(i + 1) % 2];

    error = FT_Property_Set(library,
                            "cff",
                            "hinting-engine",
                            &cffHintingEngineOther);
    if (error)
      cffHintingEngineOther = -1;

    // reset
    FT_Property_Set(library,
                    "cff",
                    "hinting-engine",
                    &cffHintingEngineDefault);
  }

  // TrueType
  error = FT_Property_Get(library,
                          "truetype",
                          "interpreter-version",
                          &ttInterpreterVersionDefault);
  if (error)
  {
    // no TrueType engine
    ttInterpreterVersionDefault = -1;
    ttInterpreterVersionOther = -1;
    ttInterpreterVersionOther1 = -1;
  }
  else
  {
    int interpreters[] =
    {
      TT_INTERPRETER_VERSION_35,
      TT_INTERPRETER_VERSION_38,
      TT_INTERPRETER_VERSION_40
    };

    int i;
    for (i = 0; i < 3; i++)
      if (ttInterpreterVersionDefault == interpreters[i])
        break;

    ttInterpreterVersionOther = interpreters[(i + 1) % 3];

    error = FT_Property_Set(library,
                            "truetype",
                            "interpreter-version",
                            &ttInterpreterVersionOther);
    if (error)
      ttInterpreterVersionOther = -1;

    ttInterpreterVersionOther1 = interpreters[(i + 2) % 3];

    error = FT_Property_Set(library,
                            "truetype",
                            "interpreter-version",
                            &ttInterpreterVersionOther1);
    if (error)
      ttInterpreterVersionOther1 = -1;

    // reset
    FT_Property_Set(library,
                    "truetype",
                    "interpreter-version",
                    &ttInterpreterVersionDefault);
  }

  // auto-hinter
  error = FT_Property_Get(library,
                          "autofitter",
                          "warping",
                          &doWarping);
  if (error)
  {
    // no warping
    haveWarping = 0;
    doWarping = 0;
  }
  else
  {
    haveWarping = 1;
    doWarping = 0; // we don't do warping by default

    FT_Property_Set(library,
                    "autofitter",
                    "warping",
                    &doWarping);
  }
}


Engine::~Engine()
{
  FTC_Manager_Done(cacheManager);
  FT_Done_FreeType(library);
}


long
Engine::numberOfFaces(int fontIndex)
{
  FT_Face face;
  long numFaces = -1;

  // search triplet (fontIndex, 0, 0)
  FTC_FaceID ftcFaceID = reinterpret_cast<void*>
                           (faceIDMap.value(FaceID(fontIndex,
                                                   0,
                                                   0)));
  if (ftcFaceID)
  {
    // found
    if (!FTC_Manager_LookupFace(cacheManager, ftcFaceID, &face))
      numFaces = face->num_faces;
  }
  else
  {
    // not found; try to load triplet (fontIndex, 0, 0)
    ftcFaceID = reinterpret_cast<void*>(faceCounter);
    faceIDMap.insert(FaceID(fontIndex, 0, 0),
                     faceCounter++);

    if (!FTC_Manager_LookupFace(cacheManager, ftcFaceID, &face))
      numFaces = face->num_faces;
    else
    {
      faceIDMap.remove(FaceID(fontIndex, 0, 0));
      faceCounter--;
    }
  }

  return numFaces;
}


int
Engine::numberOfNamedInstances(int fontIndex,
                               long faceIndex)
{
  FT_Face face;
  // we return `n' named instances plus one;
  // instance index 0 represents a face without a named instance selected
  int numNamedInstances = -1;

  // search triplet (fontIndex, faceIndex, 0)
  FTC_FaceID ftcFaceID = reinterpret_cast<void*>
                           (faceIDMap.value(FaceID(fontIndex,
                                                   faceIndex,
                                                   0)));
  if (ftcFaceID)
  {
    // found
    if (!FTC_Manager_LookupFace(cacheManager, ftcFaceID, &face))
      numNamedInstances = static_cast<int>((face->style_flags >> 16) + 1);
  }
  else
  {
    // not found; try to load triplet (fontIndex, faceIndex, 0)
    ftcFaceID = reinterpret_cast<void*>(faceCounter);
    faceIDMap.insert(FaceID(fontIndex, faceIndex, 0),
                     faceCounter++);

    if (!FTC_Manager_LookupFace(cacheManager, ftcFaceID, &face))
      numNamedInstances = static_cast<int>((face->style_flags >> 16) + 1);
    else
    {
      faceIDMap.remove(FaceID(fontIndex, faceIndex, 0));
      faceCounter--;
    }
  }

  return numNamedInstances;
}


int
Engine::loadFont(int fontIndex,
                 long faceIndex,
                 int namedInstanceIndex)
{
  int numGlyphs = -1;
  fontType = FontType_Other;

  update();

  // search triplet (fontIndex, faceIndex, namedInstanceIndex)
  scaler.face_id = reinterpret_cast<void*>
                     (faceIDMap.value(FaceID(fontIndex,
                                             faceIndex,
                                             namedInstanceIndex)));
  if (scaler.face_id)
  {
    // found
    if (!FTC_Manager_LookupSize(cacheManager, &scaler, &ftSize))
      numGlyphs = ftSize->face->num_glyphs;
  }
  else
  {
    // not found; try to load triplet
    // (fontIndex, faceIndex, namedInstanceIndex)
    scaler.face_id = reinterpret_cast<void*>(faceCounter);
    faceIDMap.insert(FaceID(fontIndex,
                            faceIndex,
                            namedInstanceIndex),
                     faceCounter++);

    if (!FTC_Manager_LookupSize(cacheManager, &scaler, &ftSize))
      numGlyphs = ftSize->face->num_glyphs;
    else
    {
      faceIDMap.remove(FaceID(fontIndex,
                              faceIndex,
                              namedInstanceIndex));
      faceCounter--;
    }
  }

  if (numGlyphs < 0)
  {
    ftSize = NULL;
    curFamilyName = QString();
    curStyleName = QString();
  }
  else
  {
    curFamilyName = QString(ftSize->face->family_name);
    curStyleName = QString(ftSize->face->style_name);

    FT_Module module = &ftSize->face->driver->root;
    const char* moduleName = module->clazz->module_name;

    // XXX cover all available modules
    if (!strcmp(moduleName, "cff"))
      fontType = FontType_CFF;
    else if (!strcmp(moduleName, "truetype"))
      fontType = FontType_TrueType;
  }

  return numGlyphs;
}


void
Engine::removeFont(int fontIndex)
{
  // we iterate over all triplets that contain the given font index
  // and remove them
  QMap<FaceID, int>::iterator iter
    = faceIDMap.lowerBound(FaceID(fontIndex, 0, 0));

  for (;;)
  {
    if (iter == faceIDMap.end())
      break;

    FaceID faceID = iter.key();
    if (faceID.fontIndex != fontIndex)
      break;

    FTC_FaceID ftcFaceID = reinterpret_cast<void*>(iter.value());
    FTC_Manager_RemoveFaceID(cacheManager, ftcFaceID);

    iter = faceIDMap.erase(iter);
  }
}


const QString&
Engine::currentFamilyName()
{
  return curFamilyName;
}


const QString&
Engine::currentStyleName()
{
  return curStyleName;
}


QString
Engine::glyphName(int index)
{
  QString name;

  if (index < 0)
    throw std::runtime_error("Invalid glyph index");

  if (ftSize && FT_HAS_GLYPH_NAMES(ftSize->face))
  {
    char buffer[256];
    if (!FT_Get_Glyph_Name(ftSize->face,
                           static_cast<unsigned int>(index),
                           buffer,
                           sizeof(buffer)))
      name = QString(buffer);
  }

  return name;
}


FT_Outline*
Engine::loadOutline(int glyphIndex)
{
  update();

  if (glyphIndex < 0)
    throw std::runtime_error("Invalid glyph index");

  FT_Glyph glyph;

  // XXX handle bitmap fonts

  // the `scaler' object is set up by the
  // `update' and `loadFont' methods
  if (FTC_ImageCache_LookupScaler(imageCache,
                                  &scaler,
                                  loadFlags | FT_LOAD_NO_BITMAP,
                                  static_cast<unsigned int>(glyphIndex),
                                  &glyph,
                                  NULL))
  {
    // XXX error handling?
    return NULL;
  }

  if (glyph->format != FT_GLYPH_FORMAT_OUTLINE)
    return NULL;

  FT_OutlineGlyph outlineGlyph = reinterpret_cast<FT_OutlineGlyph>(glyph);

  return &outlineGlyph->outline;
}


void
Engine::setCFFHintingMode(int mode)
{
  int index = gui->hintingModesCFFHash.key(mode);

  FT_Error error = FT_Property_Set(library,
                                   "cff",
                                   "hinting-engine",
                                   &index);
  if (!error)
  {
    // reset the cache
    FTC_Manager_Reset(cacheManager);
  }
}


void
Engine::setTTInterpreterVersion(int mode)
{
  int index = gui->hintingModesTrueTypeHash.key(mode);

  FT_Error error = FT_Property_Set(library,
                                   "truetype",
                                   "interpreter-version",
                                   &index);
  if (!error)
  {
    // reset the cache
    FTC_Manager_Reset(cacheManager);
  }
}


void
Engine::update()
{
  // Spinbox value cannot become negative
  dpi = static_cast<unsigned int>(gui->dpiSpinBox->value());

  if (gui->unitsComboBox->currentIndex() == MainGUI::Units_px)
  {
    pixelSize = gui->sizeDoubleSpinBox->value();
    pointSize = pixelSize * 72.0 / dpi;
  }
  else
  {
    pointSize = gui->sizeDoubleSpinBox->value();
    pixelSize = pointSize * dpi / 72.0;
  }

  doHinting = gui->hintingCheckBox->isChecked();

  doAutoHinting = gui->autoHintingCheckBox->isChecked();
  doHorizontalHinting = gui->horizontalHintingCheckBox->isChecked();
  doVerticalHinting = gui->verticalHintingCheckBox->isChecked();
  doBlueZoneHinting = gui->blueZoneHintingCheckBox->isChecked();
  showSegments = gui->segmentDrawingCheckBox->isChecked();
  doWarping = gui->warpingCheckBox->isChecked();

  gamma = gui->gammaSlider->value();

  loadFlags = FT_LOAD_DEFAULT;
  if (doAutoHinting)
    loadFlags |= FT_LOAD_FORCE_AUTOHINT;
  loadFlags |= FT_LOAD_NO_BITMAP; // XXX handle bitmap fonts also

  int index = gui->antiAliasingComboBoxx->currentIndex();

  if (doHinting)
  {
    unsigned long target;

    if (index == MainGUI::AntiAliasing_None)
      target = FT_LOAD_TARGET_MONO;
    else
    {
      switch (index)
      {
      case MainGUI::AntiAliasing_Light:
        target = FT_LOAD_TARGET_LIGHT;
        break;

      case MainGUI::AntiAliasing_LCD:
      case MainGUI::AntiAliasing_LCD_BGR:
        target = FT_LOAD_TARGET_LCD;
        break;

      case MainGUI::AntiAliasing_LCD_Vertical:
      case MainGUI::AntiAliasing_LCD_Vertical_BGR:
        target = FT_LOAD_TARGET_LCD_V;
        break;

      default:
        target = FT_LOAD_TARGET_NORMAL;
      }
    }

    loadFlags |= target;
  }
  else
  {
    loadFlags |= FT_LOAD_NO_HINTING;

    if (index == MainGUI::AntiAliasing_None)
      loadFlags |= FT_LOAD_MONOCHROME;
  }

  // XXX handle color fonts also

  scaler.pixel = 0; // use 26.6 format

  if (gui->unitsComboBox->currentIndex() == MainGUI::Units_px)
  {
    scaler.width = static_cast<unsigned int>(pixelSize * 64.0);
    scaler.height = static_cast<unsigned int>(pixelSize * 64.0);
    scaler.x_res = 0;
    scaler.y_res = 0;
  }
  else
  {
    scaler.width = static_cast<unsigned int>(pointSize * 64.0);
    scaler.height = static_cast<unsigned int>(pointSize * 64.0);
    scaler.x_res = dpi;
    scaler.y_res = dpi;
  }
}


// end of engine.cpp
