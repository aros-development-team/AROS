/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2013 by TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id$

***************************************************************************/

#include <string.h>

#include <clib/alib_protos.h>

#include <intuition/pointerclass.h>

#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#include "private.h"
#include "Debug.h"

// the meta data of the pointer image/data
#define selectPointerWidth   7
#define selectPointerHeight  16
#define selectPointerXOffset -3
#define selectPointerYOffset -8

#if defined(__amigaos4__)
// a 32bit ARGB pointer image where we can define every color of the pixels
// on our own and put an alpha-channel information in it as well.
static const ULONG selectPointer[] =
{
  0xff6d6d6d, 0xff000000, 0x00000000, 0x00000000, 0xffbbbbbb, 0xff6d6d6d, 0xff000000, // +#  :+#
  0x00000000, 0xffbbbbbb, 0xff000000, 0xffbbbbbb, 0xff000000, 0x00000000, 0x00000000, //  :#:#
  0x00000000, 0x00000000, 0xffbbbbbb, 0xff000000, 0x00000000, 0x00000000, 0x00000000, //   :#
  0x00000000, 0x00000000, 0xffbbbbbb, 0xff000000, 0x00000000, 0x00000000, 0x00000000, //   :#
  0x00000000, 0x00000000, 0xffbbbbbb, 0xff000000, 0x00000000, 0x00000000, 0x00000000, //   :#
  0x00000000, 0x00000000, 0xffbbbbbb, 0xff000000, 0x00000000, 0x00000000, 0x00000000, //   :#
  0x00000000, 0x00000000, 0xffbbbbbb, 0xff000000, 0x00000000, 0x00000000, 0x00000000, //   :#
  0x00000000, 0x00000000, 0xffbbbbbb, 0xff000000, 0x00000000, 0x00000000, 0x00000000, //   :#
  0x00000000, 0xffbbbbbb, 0xff000000, 0xff000000, 0xff000000, 0x00000000, 0x00000000, //  :###
  0x00000000, 0x00000000, 0xffbbbbbb, 0xff000000, 0x00000000, 0x00000000, 0x00000000, //   :#
  0x00000000, 0x00000000, 0xffbbbbbb, 0xff000000, 0x00000000, 0x00000000, 0x00000000, //   :#
  0x00000000, 0x00000000, 0xffbbbbbb, 0xff000000, 0x00000000, 0x00000000, 0x00000000, //   :#
  0x00000000, 0x00000000, 0xffbbbbbb, 0xff000000, 0x00000000, 0x00000000, 0x00000000, //   :#
  0x00000000, 0x00000000, 0xffbbbbbb, 0xff000000, 0x00000000, 0x00000000, 0x00000000, //   :#
  0x00000000, 0xffbbbbbb, 0xff000000, 0xffbbbbbb, 0xff000000, 0x00000000, 0x00000000, //  :#:#
  0xff6d6d6d, 0xff000000, 0x00000000, 0x00000000, 0xffbbbbbb, 0xff6d6d6d, 0xff000000, // +#  :+#
};

#ifndef POINTERA_ImageData
#define POINTERA_ImageData (POINTERA_Dummy + 0x07) // ARGB (width * height * sizeof(ULONG))
#endif
#ifndef POINTERA_Width
#define POINTERA_Width     (POINTERA_Dummy + 0x08) // <= 64
#endif
#ifndef POINTERA_Height
#define POINTERA_Height    (POINTERA_Dummy + 0x09) // <= 64
#endif
#ifndef WA_PointerType
#define WA_PointerType     (WA_Dummy + 0x50)
#endif
#ifndef POINTERTYPE_TEXT
#define POINTERTYPE_TEXT   30
#endif

#else // __amigaos4__

static const UWORD selectPointer[] =
{
//plane1    plane2
  0x0000,   0x0000,

  0x8800,   0x4600,
  0x5000,   0x2800,
  0x2000,   0x1000,
  0x2000,   0x1000,
  0x2000,   0x1000,
  0x2000,   0x1000,
  0x2000,   0x1000,
  0x2000,   0x1000,
  0x4000,   0x3800,
  0x2000,   0x1000,
  0x2000,   0x1000,
  0x2000,   0x1000,
  0x2000,   0x1000,
  0x2000,   0x1000,
  0x5000,   0x2800,
  0x8800,   0x4600,

  0x0000,   0x0000
};

#endif // __amigaos4__

// Classic bitmap data for the pointer. These will be used for OS4 aswell,
// if the graphic card cannot handle 32bit pointers.
static const UWORD selectPointer_bp0[] =
{
  0x8800,    // #...#..
  0x5000,    // .#.#...
  0x2000,    // ..#....
  0x2000,    // ..#....
  0x2000,    // ..#....
  0x2000,    // ..#....
  0x2000,    // ..#....
  0x2000,    // ..#....
  0x4000,    // .#.....
  0x2000,    // ..#....
  0x2000,    // ..#....
  0x2000,    // ..#....
  0x2000,    // ..#....
  0x2000,    // ..#....
  0x5000,    // .#.#...
  0x8800,    // #...#..
};

static const UWORD selectPointer_bp1[] =
{
  0x4600,    // .#...##
  0x2800,    // ..#.#..
  0x1000,    // ...#...
  0x1000,    // ...#...
  0x1000,    // ...#...
  0x1000,    // ...#...
  0x1000,    // ...#...
  0x1000,    // ...#...
  0x3800,    // ..###..
  0x1000,    // ...#...
  0x1000,    // ...#...
  0x1000,    // ...#...
  0x1000,    // ...#...
  0x1000,    // ...#...
  0x2800,    // ..#.#..
  0x4600,    // .#...##
};

static const UWORD selectPointer_bp2[] =
{
  0xce00,    // %#..%##
  0x7800,    // .%#%#..
  0x3000,    // ..%#...
  0x3000,    // ..%#...
  0x3000,    // ..%#...
  0x3000,    // ..%#...
  0x3000,    // ..%#...
  0x3000,    // ..%#...
  0x7800,    // .%%%#..
  0x3000,    // ..%#...
  0x3000,    // ..%#...
  0x3000,    // ..%#...
  0x3000,    // ..%#...
  0x3000,    // ..%#...
  0x7800,    // .%#%#..
  0xce00,    // %#..%##
};

static struct BitMap selectPointerBitmap =
{
  2, 16, 0, 2, 0,
  { (PLANEPTR)selectPointer_bp0, (PLANEPTR)selectPointer_bp1, NULL, NULL, NULL, NULL, NULL }
};

#if defined(__MORPHOS__)
#ifndef POINTERTYPE_SELECTTEXT
#define POINTERTYPE_SELECTTEXT 7
#endif
#ifndef WA_PointerType
#define WA_PointerType (WA_Dummy + 164)
#endif
#endif

/// IdentifyPointerColors()
static void IdentifyPointerColors(Object *obj)
{
  int i;
  ULONG colors[3*3];
  LONG blackDiff[3];
  LONG whiteDiff[3];
  LONG blackIndex;
  LONG whiteIndex;

  ENTER();

  // get the current screen's pointer colors (17 to 19)
  GetRGB32(_window(obj)->WScreen->ViewPort.ColorMap, 17, 3, colors);

  for(i=0; i < 3; i++)
  {
    LONG dr;
    LONG dg;
    LONG db;

    // normalize the colors to 8 bit per gun as GetRGB32() returns
    // 32bit left aligned values
    colors[i*3+0] >>= 24;
    colors[i*3+1] >>= 24;
    colors[i*3+2] >>= 24;

    // calculate the geometric difference to the color black (=0x00000000)
    dr = 0x00000000 - colors[i*3+0];
    dg = 0x00000000 - colors[i*3+1];
    db = 0x00000000 - colors[i*3+2];
    blackDiff[i] = dr * dr + dg * dg + db * db;

    // calculate the geometric difference to the color white (=0x000000ff)
    dr = 0x000000ff - colors[i*3+0];
    dg = 0x000000ff - colors[i*3+1];
    db = 0x000000ff - colors[i*3+2];
    whiteDiff[i] = dr * dr + dg * dg + db * db;
  }

  // the smallest difference defines the color which is closest to black or
  // equal to black
  if(blackDiff[0] > blackDiff[1])
  {
    if(blackDiff[1] > blackDiff[2])
      blackIndex = 19;
    else
      blackIndex = 18;
  }
  else if(blackDiff[0] > blackDiff[2])
    blackIndex = 19;
  else
    blackIndex = 17;

  // the smallest difference defines the color which is closest to white or
  // equal to white
  if(whiteDiff[0] > whiteDiff[1])
  {
    if(whiteDiff[1] > whiteDiff[2])
      whiteIndex = 19;
    else
      whiteIndex = 18;
  }
  else if(whiteDiff[0] > whiteDiff[2])
    whiteIndex = 19;
  else
    whiteIndex = 17;

  // Here we expect the user to have set up quite "different" colors. That
  // means the color closest to white will never be close to black and vice
  // versa. According to these differences we spread the required bitplanes.
  if(whiteIndex == 17)
  {
    if(blackIndex == 18)
    {
      selectPointerBitmap.Planes[0] = (PLANEPTR)selectPointer_bp0;
      selectPointerBitmap.Planes[1] = (PLANEPTR)selectPointer_bp1;
    }
    else // blackIndex == 19
    {
      selectPointerBitmap.Planes[0] = (PLANEPTR)selectPointer_bp2;
      selectPointerBitmap.Planes[1] = (PLANEPTR)selectPointer_bp1;
    }
  }
  else if(whiteIndex == 18)
  {
    if(blackIndex == 17)
    {
      selectPointerBitmap.Planes[0] = (PLANEPTR)selectPointer_bp1;
      selectPointerBitmap.Planes[1] = (PLANEPTR)selectPointer_bp0;
    }
    else // blackIndex == 19
    {
      selectPointerBitmap.Planes[0] = (PLANEPTR)selectPointer_bp1;
      selectPointerBitmap.Planes[1] = (PLANEPTR)selectPointer_bp2;
    }
  }
  else // whiteIndex == 19
  {
    if(blackIndex == 17)
    {
      selectPointerBitmap.Planes[0] = (PLANEPTR)selectPointer_bp2;
      selectPointerBitmap.Planes[1] = (PLANEPTR)selectPointer_bp0;
    }
    else // blackIndex == 18
    {
      selectPointerBitmap.Planes[0] = (PLANEPTR)selectPointer_bp0;
      selectPointerBitmap.Planes[1] = (PLANEPTR)selectPointer_bp2;
    }
  }

  LEAVE();
}

///
/// SetupSelectPointer()
void SetupSelectPointer(struct InstData *data)
{
  ENTER();

  #if defined(__amigaos4__)
  if(LIB_VERSION_IS_AT_LEAST(IntuitionBase, 53, 40))
    data->PointerObj = (APTR)POINTERTYPE_TEXT;
  #elif defined(__MORPHOS__)
  if(IS_MORPHOS2)
    data->PointerObj = (APTR)POINTERTYPE_SELECTTEXT;
  #endif

  if(data->PointerObj == NULL)
  {
    #if defined(__amigaos4__)
    data->PointerObj = (Object *)NewObject(NULL, "pointerclass",
      POINTERA_ImageData,   selectPointer,
      POINTERA_Width,       selectPointerWidth,
      POINTERA_Height,      selectPointerHeight,
      POINTERA_BitMap,      &selectPointerBitmap,
      POINTERA_WordWidth,   (ULONG)1,
      POINTERA_XResolution, (ULONG)POINTERXRESN_SCREENRES,
      POINTERA_YResolution, (ULONG)POINTERYRESN_SCREENRESASPECT,
      POINTERA_XOffset,     (LONG)selectPointerXOffset,
      POINTERA_YOffset,     (LONG)selectPointerYOffset,
      TAG_DONE);
    #else
    if(LIB_VERSION_IS_AT_LEAST(IntuitionBase, 39, 0))
    {
      data->PointerObj = (Object *)NewObject(NULL, (STRPTR)"pointerclass",
        POINTERA_BitMap,      (SIPTR)&selectPointerBitmap,
        POINTERA_WordWidth,   (ULONG)1,
        POINTERA_XResolution, (ULONG)POINTERXRESN_SCREENRES,
        POINTERA_YResolution, (ULONG)POINTERYRESN_SCREENRESASPECT,
        POINTERA_XOffset,     (LONG)selectPointerXOffset,
        POINTERA_YOffset,     (LONG)selectPointerYOffset,
        TAG_DONE);
    }
    else
    {
      if((data->PointerObj = (Object *)AllocVec(sizeof(selectPointer), MEMF_CHIP|MEMF_PUBLIC)) != NULL)
        memcpy(data->PointerObj, selectPointer, sizeof(selectPointer));
    }
    #endif
  }

  data->activeSelectPointer = FALSE;

  LEAVE();
}

///
/// CleanupSelectPointer()
void CleanupSelectPointer(struct InstData *data)
{
  ENTER();

  #if defined(__amigaos4__)
  if(LIB_VERSION_IS_AT_LEAST(IntuitionBase, 53, 40))
    data->PointerObj = NULL;
  #elif defined(__MORPHOS__)
  if (IS_MORPHOS2)
    data->PointerObj = NULL;
  #endif

  if(data->PointerObj != NULL)
  {
    #if defined(DEBUG)
    if(data->activeSelectPointer == TRUE)
      E(DBF_ALWAYS, "pointer was still active upon MUIM_Cleanup!!");
    #endif

    #if defined(__amigaos4__)
    DisposeObject(data->PointerObj);
    #else
    if(LIB_VERSION_IS_AT_LEAST(IntuitionBase, 39, 0))
    {
      DisposeObject(data->PointerObj);
    }
    else
    {
      FreeVec(data->PointerObj);
    }
    #endif

    data->PointerObj = NULL;
  }

  LEAVE();
}

///
/// ShowSelectPointer()
void ShowSelectPointer(struct InstData *data, Object *obj)
{
  ENTER();

  // even if it seems to be a waste of performance, but
  // we unfortunately have to always set the window pointer
  // regardless of the point if it was previously set or not.
  // This is required as any other gadget or process might
  // reset the window pointer and as such we would end up
  // with no custom pointer as well. So we only check the window
  // sleep status here :(
  if(data->PointerObj != NULL &&
     xget(_win(obj), MUIA_Window_Sleep) == FALSE)
  {
    // try to identify the black/white colors
    // of the current screen colormap
    if(data->activeSelectPointer == FALSE)
      IdentifyPointerColors(obj);

    // now we set the actual new custom window pointer. Please note
    // that we can't unfortunately check for data->activeSelectPointer
    // here because otherwise we might end up with the standard
    // window pointer when quickly switching pointer TE.mcc
    #if defined(__amigaos4__)
    SetWindowPointer(_window(obj), LIB_VERSION_IS_AT_LEAST(IntuitionBase, 53, 407) ? WA_PointerType : WA_Pointer, data->PointerObj, TAG_DONE);
    #elif defined(__MORPHOS__)
    SetWindowPointer(_window(obj), IS_MORPHOS2 ? WA_PointerType : WA_Pointer, data->PointerObj, TAG_DONE);
    #else
    if(LIB_VERSION_IS_AT_LEAST(IntuitionBase, 39, 0))
      SetWindowPointer(_window(obj), WA_Pointer, data->PointerObj, TAG_DONE);
    else
      SetPointer(_window(obj), (APTR)data->PointerObj, selectPointerHeight,
                                                       selectPointerWidth,
                                                       selectPointerXOffset,
                                                       selectPointerYOffset);
    #endif

    data->activeSelectPointer = TRUE;
  }

  LEAVE();
}

///
/// HideSelectPointer()
void HideSelectPointer(struct InstData *data, Object *obj)
{
  ENTER();

  if(data->activeSelectPointer == TRUE &&
     data->PointerObj != NULL)
  {
    #if defined(__amigaos4__) || defined(__MORPHOS__)
    SetWindowPointer(_window(obj), TAG_DONE);
    #else
    if(LIB_VERSION_IS_AT_LEAST(IntuitionBase, 39, 0))
      SetWindowPointer(_window(obj), TAG_DONE);
    else
      ClearPointer(_window(obj));
    #endif

    data->activeSelectPointer = FALSE;
  }

  LEAVE();
}

///
