/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by BetterString.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/

 $Id: private.h,v 1.1 2005/04/21 20:52:04 damato Exp $

***************************************************************************/

#ifndef BETTERSTRING_MCP_PRIV_H
#define BETTERSTRING_MCP_PRIV_H

#include "BetterString_mcp.h"

#include <mcc_common.h>
#include <mcc_debug.h>

#define PREFSIMAGEOBJECT \
  BitmapObject,\
    MUIA_Bitmap_Bitmap,       (UBYTE *)&image_bitmap,\
    MUIA_Bitmap_Height,       IMAGE_HEIGHT,\
    MUIA_Bitmap_Precision,    0,\
    MUIA_Bitmap_SourceColors, (ULONG *)image_palette,\
    MUIA_Bitmap_Transparent,  0,\
    MUIA_Bitmap_Width,        IMAGE_WIDTH,\
    MUIA_FixHeight,           IMAGE_HEIGHT,\
    MUIA_FixWidth,            IMAGE_WIDTH,\
  End

#define MCPMAXRAWBUF 64

#define IEQUALIFIER_SHIFT   0x0200
#define IEQUALIFIER_ALT     0x0400
#define IEQUALIFIER_COMMAND 0x0800

enum
{
	ActiveBack = 0,
	ActiveText,
	InactiveBack,
	InactiveText,
	Cursor,
	MarkedBack,
	MarkedText,
	Font,
	Frame,

	NumberOfObject
};

struct InstData_MCP
{
	Object *Objects[NumberOfObject];
};

Object *CreatePrefsGroup(struct InstData_MCP *data);

#endif /* BETTERSTRING_MCP_PRIV_H */
