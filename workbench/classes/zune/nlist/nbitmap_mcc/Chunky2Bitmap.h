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

#ifndef CHUNKY2BITMAP_H
#define CHUNKY2BITMAP_H 1

#ifndef EXEC_TYPES_H
  #include <exec/types.h>
#endif
#ifndef GRAPHICS_GFX_H
  #include <graphics/gfx.h>
#endif

struct BitMap *Chunky2Bitmap(APTR chunky, ULONG width, ULONG height, ULONG depth);

#endif /* CHUNKY2BITMAP_H */
