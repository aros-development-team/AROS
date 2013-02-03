/***************************************************************************

 NBitmap.mcc - New Bitmap MUI Custom Class
 Copyright (C) 2006 by Daniel Allsopp
 Copyright (C) 2007 by NList Open Source Team

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

#ifndef NBITMAP_MCC_PRIV_H
#define NBITMAP_MCC_PRIV_H

/* system includes */
#include <dos/exall.h>
#include <exec/types.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>

/* mcc includes */
#include <mcc_common.h>

/* local includes */
#include "Debug.h"
#include "NBitmap.h"

/* private definitions */
#define MUIV_NBitmap_Normal       0
#define MUIV_NBitmap_Ghosted      1
#define MUIV_NBitmap_Selected     2

#ifndef uint8
typedef UBYTE uint8;
#endif
#ifndef int8
typedef BYTE int8;
#endif
#ifndef uint16
typedef UWORD uint16;
#endif
#ifndef int16
typedef WORD int16;
#endif
#ifndef uint32
typedef ULONG uint32;
#endif
#ifndef int32
typedef LONG int32;
#endif

/* private structures */
struct PrefsData
{
  uint8 show_label;
  uint8 overlay_type;
  uint8 overlay_r;
  uint8 overlay_g;
  uint8 overlay_b;
  uint16 overlay_shadeover;
  uint16 overlay_shadepress;
  uint8 spacing_horiz;
  uint8 spacing_vert;
};

struct InstData
{
  BOOL button;
  BOOL overlay;
  BOOL pressed;
  STRPTR label;

  uint32 scrdepth;

  uint32 *data[3];
  uint32 fmt, type;
  uint32 width, height, depth;
  uint32 maxwidth, maxheight;
  uint32 border_horiz, border_vert;
  uint32 label_horiz, label_vert;
  uint32 arraybpp, arraybpr, arraysize;
  APTR arraypixels[3];
  uint32 shadeWidth;
  uint32 shadeHeight;
  uint32 shadeBytesPerRow;
  APTR overShadePixels;
  APTR pressedShadePixels;

  uint32 *dt_colours[3];
  Object *dt_obj[3];
  PLANEPTR *dt_mask[3];

  struct PrefsData prefs;
  struct BitMap *dt_bitmap[3];
  struct BitMapHeader *dt_header[3];
  struct TextExtent labelte;

  uint32 alpha;
  const uint32 *clut;
  APTR ditheredImage[3];
  APTR ditheredMask[3];
  int32 ditherPenMap[256];
  #if !defined(__amigaos4__)
  struct BitMap *ditheredBitmap[3];
  #endif

  struct MUI_EventHandlerNode ehnode;
};

/* macros */
#define _id(obj) (muiNotifyData(obj)->mnd_ObjectID)
#define _between(a,x,b) ((x)>=(a) && (x)<=(b))
#define _isinobject(x,y) (_between(_mleft(obj),(x),_mright(obj)) && _between(_mtop(obj),(y),_mbottom(obj)))

#ifndef min
#define min(a, b) (((a) < (b)) ? (a):(b))
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a):(b))
#endif

#ifndef MUI_EHF_GUIMODE
#define MUI_EHF_GUIMODE     (1<<1)  /* set this if you dont want your handler to be called when your object is disabled or invisible */
#endif

/* prototypes */
BOOL NBitmap_LoadImage(STRPTR filename, uint32 item, struct IClass *cl, Object *obj);
VOID NBitmap_UpdateImage(uint32 item, STRPTR filename, struct IClass *cl, Object *obj);
//BOOL NBitmap_ExamineData(Object *dt_obj, uint32 item, struct IClass *cl, Object *obj);
//VOID NBitmap_FreeImage(uint32 item, struct IClass *cl, Object *obj);
BOOL NBitmap_NewImage(struct IClass *cl, Object *obj);
VOID NBitmap_DisposeImage(struct IClass *cl, Object *obj);
BOOL NBitmap_SetupImage(struct IClass *cl, Object *obj);
VOID NBitmap_CleanupImage(struct IClass *cl, Object *obj);
VOID NBitmap_DrawImage(struct IClass *cl, Object *obj);
BOOL NBitmap_SetupShades(struct InstData *data);
void NBitmap_CleanupShades(struct InstData *data);

#if !defined(__amigaos4__) && !defined(__MORPHOS__) && !defined(__AROS__)
ULONG _WPA(APTR src, UWORD srcx, UWORD srcy, UWORD srcmod, struct RastPort *rp, UWORD destx, UWORD desty, UWORD width, UWORD height, ULONG fmt);
ULONG _WPAA(APTR src, UWORD srcx, UWORD srcy, UWORD srcmod, struct RastPort *rp, UWORD destx, UWORD desty, UWORD width, UWORD height, ULONG globalalpha);
#endif

#ifndef PDTA_AlphaChannel
/* Seems this V43 extension is not in the Amiga SDK */
#define PDTA_AlphaChannel     (DTA_Dummy + 256)
#endif

#endif /* NBITMAP_MCC_PRIV_H */
