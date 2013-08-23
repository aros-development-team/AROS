/***************************************************************************

 NBalance.mcc - New Balance MUI Custom Class
 Copyright (C) 2008-2013 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

#include <string.h>

#include <clib/alib_protos.h>

#include <intuition/pointerclass.h>

#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <libraries/mui.h>

#include "private.h"
#include "muiextra.h"

// the meta data of our pointers
#define horizSizePointerWidth      16
#define horizSizePointerHeight     16
#define horizSizePointerXOffset    -8
#define horizSizePointerYOffset    -8

#define vertSizePointerWidth       16
#define vertSizePointerHeight      16
#define vertSizePointerXOffset     -8
#define vertSizePointerYOffset     -8

#if defined(__amigaos4__)
// a 32bit ARGB pointer image where we can define every color of the pixels
// on our own and put an alpha-channel information in it as well.
static const ULONG horizSizePointer[] =
{
  /* ......++++...... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* ......+##+...... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* ......+##+...... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* ......+##+...... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* ...++.+##+.++... */ 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000,
  /* ..+#+.+##+.+#+.. */ 0x00000000, 0x00000000, 0xffffffff, 0xff000000, 0xffffffff, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0xffffffff, 0xff000000, 0xffffffff, 0x00000000, 0x00000000,
  /* .+##+++##+++##+. */ 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0x00000000,
  /* +##############+ */ 0xffffffff, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xffffffff,
  /* +##############+ */ 0xffffffff, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xffffffff,
  /* .+##+++##+++##+. */ 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0x00000000,
  /* ..+#+.+##+.+#+.. */ 0x00000000, 0x00000000, 0xffffffff, 0xff000000, 0xffffffff, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0xffffffff, 0xff000000, 0xffffffff, 0x00000000, 0x00000000,
  /* ...++.+##+.++... */ 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000,
  /* ......+##+...... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* ......+##+...... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* ......+##+...... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* ......++++...... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

static const ULONG vertSizePointer[] =
{
  /* .......++....... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* ......+##+...... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* .....+####+..... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* ....+######+.... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* ....+++##+++.... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* ......+##+...... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* +++++++##+++++++ */ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
  /* +##############+ */ 0xffffffff, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xffffffff,
  /* +##############+ */ 0xffffffff, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xffffffff,
  /* +++++++##+++++++ */ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
  /* ......+##+...... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* ....+++##+++.... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* ....+######+.... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* .....+####+..... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* ......+##+...... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xff000000, 0xff000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  /* .......++....... */ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
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
#ifndef POINTERTYPE_COLUMNRESIZE
#define POINTERTYPE_COLUMNRESIZE   4
#endif
#ifndef POINTERTYPE_ROWRESIZE
#define POINTERTYPE_ROWRESIZE      25
#endif

#else // __amigaos4__

#if defined(__MORPHOS__)
#ifndef POINTERTYPE_VERTICALRESIZE
#define POINTERTYPE_VERTICALRESIZE   10
#define POINTERTYPE_HORIZONTALRESIZE 11
#endif
#ifndef WA_PointerType
#define WA_PointerType (WA_Dummy + 164)
#endif
#endif

static const UWORD horizSizePointer[] =
{
//plane1    plane2
  0x0000,   0x0000,

  0x03c0,   0x0000,
  0x0240,   0x0180,
  0x0240,   0x0180,
  0x0240,   0x0180,
  0x1a58,   0x0180,
  0x2a54,   0x1008,
  0x4e72,   0x300c,
  0x8001,   0x7ffe,
  0x8001,   0x7ffe,
  0x4e72,   0x300c,
  0x2a54,   0x0180,
  0x1a58,   0x0180,
  0x0240,   0x0180,
  0x0240,   0x0180,
  0x0240,   0x0180,
  0x03c0,   0x0000,

  0x0000,   0x0000
};

static const UWORD vertSizePointer[] =
{
//plane1    plane2
  0x0000,   0x0000,

  0x0180,   0x0000,
  0x0240,   0x0180,
  0x0420,   0x03c0,
  0x0810,   0x07e0,
  0x0e70,   0x0180,
  0x0240,   0x0180,
  0xfe7f,   0x0180,
  0x8001,   0x7ffe,
  0x8001,   0x7ffe,
  0xfe7f,   0x0180,
  0x0240,   0x0180,
  0x0e70,   0x0180,
  0x0810,   0x07e0,
  0x0420,   0x03c0,
  0x0240,   0x0180,
  0x0180,   0x0000,

  0x0000,   0x0000
};

#endif // __amigaos4__

// Classic bitmap data for the pointers. These will be used for OS4 aswell,
// if the graphic card cannot handle 32bit pointers.

// the horizontal size pointer
static const UWORD horizSizePointer_bp0[] =
{
  /* ......++++...... */ 0x03c0,
  /* ......+..+...... */ 0x0240,
  /* ......+..+...... */ 0x0240,
  /* ......+..+...... */ 0x0240,
  /* ...++.+..+.++... */ 0x1a58,
  /* ..+.+.+..+.+.+.. */ 0x2a54,
  /* .+..+++..+++..+. */ 0x4e72,
  /* +..............+ */ 0x8001,
  /* +..............+ */ 0x8001,
  /* .+..+++..+++..+. */ 0x4e72,
  /* ..+.+.+..+.+.+.. */ 0x2a54,
  /* ...++.+..+.++... */ 0x1a58,
  /* ......+..+...... */ 0x0240,
  /* ......+..+...... */ 0x0240,
  /* ......+..+...... */ 0x0240,
  /* ......++++...... */ 0x03c0,
};

static const UWORD horizSizePointer_bp1[] =
{
  /* ................ */ 0x0000,
  /* .......##....... */ 0x0180,
  /* .......##....... */ 0x0180,
  /* .......##....... */ 0x0180,
  /* .......##....... */ 0x0180,
  /* ...#...##...#... */ 0x1188,
  /* ..##...##...##.. */ 0x318c,
  /* .##############. */ 0x7ffe,
  /* .##############. */ 0x7ffe,
  /* ..##...##...##.. */ 0x318c,
  /* ...#...##...#... */ 0x1188,
  /* .......##....... */ 0x0180,
  /* .......##....... */ 0x0180,
  /* .......##....... */ 0x0180,
  /* .......##....... */ 0x0180,
  /* ................ */ 0x0000,
};

static const UWORD horizSizePointer_bp2[] =
{
  /* ......++++...... */ 0x03c0,
  /* ......+##+...... */ 0x03c0,
  /* ......+##+...... */ 0x03c0,
  /* ......+##+...... */ 0x03c0,
  /* ...++.+##+.++... */ 0x1bd8,
  /* ..+#+.+##+.+#+.. */ 0x3bdc,
  /* .+##+++##+++##+. */ 0x7ffe,
  /* +##############+ */ 0xffff,
  /* +##############+ */ 0xffff,
  /* .+##+++##+++##+. */ 0x7ffe,
  /* ..+#+.+##+.+#+.. */ 0x3bdc,
  /* ...++.+##+.++... */ 0x1bd8,
  /* ......+##+...... */ 0x03c0,
  /* ......+##+...... */ 0x03c0,
  /* ......+##+...... */ 0x03c0,
  /* ......++++...... */ 0x03c0,
};

static struct BitMap horizSizePointerBitmap =
{
  2, 16, 0, 2, 0,
  { (PLANEPTR)horizSizePointer_bp0, (PLANEPTR)horizSizePointer_bp1, NULL, NULL, NULL, NULL, NULL, NULL }
};

// the vertical size pointer
static const UWORD vertSizePointer_bp0[] =
{
  /* .......++....... */ 0x0180,
  /* ......+..+...... */ 0x0240,
  /* .....+....+..... */ 0x0420,
  /* ....+......+.... */ 0x0810,
  /* ....+++..+++.... */ 0x0e70,
  /* ......+..+...... */ 0x0240,
  /* +++++++..+++++++ */ 0xfe7f,
  /* +..............+ */ 0x8001,
  /* +..............+ */ 0x8001,
  /* +++++++..+++++++ */ 0xfe7f,
  /* ......+..+...... */ 0x0240,
  /* ....+++..+++.... */ 0x0e70,
  /* ....+......+.... */ 0x0810,
  /* .....+....+..... */ 0x0420,
  /* ......+..+...... */ 0x0240,
  /* .......++....... */ 0x0180,
};

static const UWORD vertSizePointer_bp1[] =
{
  /* ................ */ 0x0000,
  /* .......##....... */ 0x0180,
  /* ......####...... */ 0x03c0,
  /* .....######..... */ 0x07e0,
  /* .......##....... */ 0x0180,
  /* .......##....... */ 0x0180,
  /* .......##....... */ 0x0180,
  /* .##############. */ 0x7ffe,
  /* .##############. */ 0x7ffe,
  /* .......##....... */ 0x0180,
  /* .......##....... */ 0x0180,
  /* .......##....... */ 0x0180,
  /* .....######..... */ 0x07e0,
  /* ......####...... */ 0x03c0,
  /* .......##....... */ 0x0180,
  /* ................ */ 0x0000,
};

static const UWORD vertSizePointer_bp2[] =
{
  /* .......++....... */ 0x0180,
  /* ......+##+...... */ 0x03c0,
  /* .....+####+..... */ 0x07e0,
  /* ....+######+.... */ 0x0ff0,
  /* ....+++##+++.... */ 0x0ff0,
  /* ......+##+...... */ 0x03c0,
  /* +++++++##+++++++ */ 0xffff,
  /* +##############+ */ 0xffff,
  /* +##############+ */ 0xffff,
  /* +++++++##+++++++ */ 0xffff,
  /* ......+##+...... */ 0x03c0,
  /* ....+++##+++.... */ 0x0ff0,
  /* ....+######+.... */ 0x0ff0,
  /* .....+####+..... */ 0x07e0,
  /* ......+##+...... */ 0x03c0,
  /* .......++....... */ 0x0180,
};

static struct BitMap vertSizePointerBitmap =
{
  2, 16, 0, 2, 0,
  { (PLANEPTR)vertSizePointer_bp0, (PLANEPTR)vertSizePointer_bp1, NULL, NULL, NULL, NULL, NULL, NULL }
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)    (sizeof(a) / sizeof(a[0]))
#endif

static void IdentifyPointerColors(Object *obj)
{
  ULONG i;
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
      horizSizePointerBitmap.Planes[0] = (PLANEPTR)horizSizePointer_bp0;
      horizSizePointerBitmap.Planes[1] = (PLANEPTR)horizSizePointer_bp1;
      vertSizePointerBitmap.Planes[0] = (PLANEPTR)vertSizePointer_bp0;
      vertSizePointerBitmap.Planes[1] = (PLANEPTR)vertSizePointer_bp1;
    }
    else // blackIndex == 19
    {
      horizSizePointerBitmap.Planes[0] = (PLANEPTR)horizSizePointer_bp2;
      horizSizePointerBitmap.Planes[1] = (PLANEPTR)horizSizePointer_bp1;
      vertSizePointerBitmap.Planes[0] = (PLANEPTR)vertSizePointer_bp2;
      vertSizePointerBitmap.Planes[1] = (PLANEPTR)vertSizePointer_bp1;
    }
  }
  else if(whiteIndex == 18)
  {
    if(blackIndex == 17)
    {
      horizSizePointerBitmap.Planes[0] = (PLANEPTR)horizSizePointer_bp1;
      horizSizePointerBitmap.Planes[1] = (PLANEPTR)horizSizePointer_bp0;
      vertSizePointerBitmap.Planes[0] = (PLANEPTR)vertSizePointer_bp1;
      vertSizePointerBitmap.Planes[1] = (PLANEPTR)vertSizePointer_bp0;
    }
    else // blackIndex == 19
    {
      horizSizePointerBitmap.Planes[0] = (PLANEPTR)horizSizePointer_bp1;
      horizSizePointerBitmap.Planes[1] = (PLANEPTR)horizSizePointer_bp2;
      vertSizePointerBitmap.Planes[0] = (PLANEPTR)vertSizePointer_bp1;
      vertSizePointerBitmap.Planes[1] = (PLANEPTR)vertSizePointer_bp2;
    }
  }
  else // whiteIndex == 19
  {
    if(blackIndex == 17)
    {
      horizSizePointerBitmap.Planes[0] = (PLANEPTR)horizSizePointer_bp2;
      horizSizePointerBitmap.Planes[1] = (PLANEPTR)horizSizePointer_bp0;
      vertSizePointerBitmap.Planes[0] = (PLANEPTR)vertSizePointer_bp2;
      vertSizePointerBitmap.Planes[1] = (PLANEPTR)vertSizePointer_bp0;
    }
    else // blackIndex == 18
    {
      horizSizePointerBitmap.Planes[0] = (PLANEPTR)horizSizePointer_bp0;
      horizSizePointerBitmap.Planes[1] = (PLANEPTR)horizSizePointer_bp2;
      vertSizePointerBitmap.Planes[0] = (PLANEPTR)vertSizePointer_bp0;
      vertSizePointerBitmap.Planes[1] = (PLANEPTR)vertSizePointer_bp2;
    }
  }

  LEAVE();
}

void SetupCustomPointers(struct InstData *data)
{
  ENTER();

  #if defined(__amigaos4__)
  if(LIB_VERSION_IS_AT_LEAST(IntuitionBase, 53, 41))
  {
    data->horizSizePointerObj = (APTR)POINTERTYPE_COLUMNRESIZE;
    data->vertSizePointerObj = (APTR)POINTERTYPE_ROWRESIZE;
  }
  #elif defined(__MORPHOS__)
  if(LIB_VERSION_IS_AT_LEAST(IntuitionBase, 51, 0))
  {
    data->horizSizePointerObj = (APTR)POINTERTYPE_HORIZONTALRESIZE;
    data->vertSizePointerObj = (APTR)POINTERTYPE_VERTICALRESIZE;
  }
  #endif

  if(data->horizSizePointerObj == NULL)
  {
    #if defined(__amigaos4__)
    data->horizSizePointerObj = (Object *)NewObject(NULL, "pointerclass",
      POINTERA_ImageData,   horizSizePointer,
      POINTERA_Width,       horizSizePointerWidth,
      POINTERA_Height,      horizSizePointerHeight,
      POINTERA_BitMap,      (LONG)&horizSizePointerBitmap,
      POINTERA_WordWidth,   (ULONG)1,
      POINTERA_XResolution, (ULONG)POINTERXRESN_SCREENRES,
      POINTERA_YResolution, (ULONG)POINTERYRESN_SCREENRESASPECT,
      POINTERA_XOffset,     (LONG)horizSizePointerXOffset,
      POINTERA_YOffset,     (LONG)horizSizePointerYOffset,
      TAG_DONE);
    #else
    data->horizSizePointerObj = (Object *)NewObject(NULL, (STRPTR)"pointerclass",
      POINTERA_BitMap,      (IPTR)&horizSizePointerBitmap,
      POINTERA_WordWidth,   (ULONG)1,
      POINTERA_XResolution, (ULONG)POINTERXRESN_SCREENRES,
      POINTERA_YResolution, (ULONG)POINTERYRESN_SCREENRESASPECT,
      POINTERA_XOffset,     (LONG)horizSizePointerXOffset,
      POINTERA_YOffset,     (LONG)horizSizePointerYOffset,
      TAG_DONE);
    #endif
  }

  if(data->vertSizePointerObj == NULL)
  {
    #if defined(__amigaos4__)
    data->vertSizePointerObj = (Object *)NewObject(NULL, "pointerclass",
      POINTERA_ImageData,   vertSizePointer,
      POINTERA_Width,       vertSizePointerWidth,
      POINTERA_Height,      vertSizePointerHeight,
      POINTERA_BitMap,      (LONG)&vertSizePointerBitmap,
      POINTERA_WordWidth,   (ULONG)1,
      POINTERA_XResolution, (ULONG)POINTERXRESN_SCREENRES,
      POINTERA_YResolution, (ULONG)POINTERYRESN_SCREENRESASPECT,
      POINTERA_XOffset,     (LONG)vertSizePointerXOffset,
      POINTERA_YOffset,     (LONG)vertSizePointerYOffset,
      TAG_DONE);
    #else
    data->vertSizePointerObj = (Object *)NewObject(NULL, (STRPTR)"pointerclass",
      POINTERA_BitMap,      (IPTR)&vertSizePointerBitmap,
      POINTERA_WordWidth,   (ULONG)1,
      POINTERA_XResolution, (ULONG)POINTERXRESN_SCREENRES,
      POINTERA_YResolution, (ULONG)POINTERYRESN_SCREENRESASPECT,
      POINTERA_XOffset,     (LONG)vertSizePointerXOffset,
      POINTERA_YOffset,     (LONG)vertSizePointerYOffset,
      TAG_DONE);
    #endif
  }

  LEAVE();
}

void CleanupCustomPointers(struct InstData *data)
{
  ENTER();

  #if defined(__amigaos4__)
  if(LIB_VERSION_IS_AT_LEAST(IntuitionBase, 53, 41))
  {
    data->horizSizePointerObj = NULL;
    data->vertSizePointerObj = NULL;
  }
  #elif defined(__MORPHOS__)
  if(LIB_VERSION_IS_AT_LEAST(IntuitionBase, 51, 0))
  {
    data->horizSizePointerObj = NULL;
    data->vertSizePointerObj = NULL;
  }
  #endif

  // dispose the different pointer objects
  if(data->horizSizePointerObj != NULL)
  {
    DisposeObject(data->horizSizePointerObj);
    data->horizSizePointerObj = NULL;
  }

  if(data->vertSizePointerObj != NULL)
  {
    DisposeObject(data->vertSizePointerObj);
    data->vertSizePointerObj = NULL;
  }

  data->activeCustomPointer = PT_NONE;

  LEAVE();
}

void ShowCustomPointer(Object *obj, struct InstData *data)
{
  ENTER();

  // even if it seems to be a waste of performance, but
  // we unfortunately have to always set the window pointer
  // regardless of the point if it was previously set or not.
  // This is required as any other gadget or process might
  // reset the window pointer and as such we would end up
  // with no custom pointer as well. So we only check the window
  // sleep status here :(
  if(xget(_win(obj), MUIA_Window_Sleep) == FALSE)
  {
    enum PointerType type = (data->groupType == MUIV_Group_Type_Horiz) ? PT_HORIZ : PT_VERT;
    Object *ptrObject = NULL;

    switch(type)
    {
      case PT_HORIZ:
        ptrObject = data->horizSizePointerObj;
      break;

      case PT_VERT:
        ptrObject = data->vertSizePointerObj;
      break;

      case PT_NONE:
        ptrObject = NULL;
      break;
    }

    if(ptrObject != NULL)
    {
      // try to identify the black/white colors
      // of the current screen colormap
      IdentifyPointerColors(obj);

      #if defined(__amigaos4__)
      SetWindowPointer(_window(obj), LIB_VERSION_IS_AT_LEAST(IntuitionBase, 53, 41) ? WA_PointerType : WA_Pointer, ptrObject, TAG_DONE);
      #elif defined(__MORPHOS__)
      SetWindowPointer(_window(obj), LIB_VERSION_IS_AT_LEAST(IntuitionBase, 51, 0) ? WA_PointerType : WA_Pointer, ptrObject, TAG_DONE);
      #else
      SetWindowPointer(_window(obj), WA_Pointer, ptrObject, TAG_DONE);
      #endif

      data->activeCustomPointer = type;
    }
    else
      HideCustomPointer(obj, data);
  }

  LEAVE();
}

void HideCustomPointer(Object *obj, struct InstData *data)
{
  ENTER();

  if(data->activeCustomPointer != PT_NONE)
  {
    SetWindowPointer(_window(obj), TAG_DONE);
    data->activeCustomPointer = PT_NONE;
  }

  LEAVE();
}
