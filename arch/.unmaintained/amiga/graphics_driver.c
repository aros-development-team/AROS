/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG_FreeMem 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <graphics/rastport.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <clib/graphics_protos.h>
#include <clib/aros_protos.h>
#include "graphics_intern.h"

#define static	/* nothing */

extern void _aros_not_implemented(void);

int driver_init (struct GfxBase * GfxBase)
{
    fprintf(stderr, "gfx driver init function goes here\n");
    return FALSE;
}

int driver_open (struct GfxBase * GfxBase)
{
    _aros_not_implemented();
    return FALSE;
}

void driver_close (struct GfxBase * GfxBase)
{
    _aros_not_implemented();
    return;
}

void driver_expunge (struct GfxBase * GfxBase)
{
    _aros_not_implemented();
    return;
}

void driver_SetABPenDrMd (struct RastPort * rp, ULONG apen, ULONG bpen,
	ULONG drmd, struct GfxBase * GfxBase)
{
    _aros_not_implemented();
}

void driver_SetAPen (struct RastPort * rp, ULONG pen,
		    struct GfxBase * GfxBase)
{
    _aros_not_implemented();
}

void driver_SetBPen (struct RastPort * rp, ULONG pen,
		    struct GfxBase * GfxBase)
{
    _aros_not_implemented();
}

void driver_SetOutlinePen (struct RastPort * rp, ULONG pen,
		    struct GfxBase * GfxBase)
{
    _aros_not_implemented();
}

void driver_SetDrMd (struct RastPort * rp, ULONG mode,
		    struct GfxBase * GfxBase)
{
    _aros_not_implemented();
}

void driver_EraseRect (struct RastPort * rp, LONG x1, LONG y1, LONG x2, LONG y2,
		    struct GfxBase * GfxBase)
{
    _aros_not_implemented();
}

void driver_RectFill (struct RastPort * rp, LONG x1, LONG y1, LONG x2, LONG y2,
		    struct GfxBase * GfxBase)
{
    _aros_not_implemented();
}

void driver_ScrollRaster (struct RastPort * rp, LONG dx, LONG dy,
	LONG x1, LONG y1, LONG x2, LONG y2, struct GfxBase * GfxBase)
{
    _aros_not_implemented();
}

void driver_DrawEllipse (struct RastPort * rp, LONG x, LONG y, LONG rx, LONG ry,
		struct GfxBase * GfxBase)
{
    _aros_not_implemented();
}

void driver_Text (struct RastPort * rp, STRPTR string, LONG len,
		struct GfxBase * GfxBase)
{
    _aros_not_implemented();
}

WORD driver_TextLength (struct RastPort * rp, STRPTR string, ULONG len,
		    struct GfxBase * GfxBase)
{
    _aros_not_implemented();
    return 0;
}

void driver_Move (struct RastPort * rp, LONG x, LONG y,
		    struct GfxBase * GfxBase)
{
    _aros_not_implemented();
    return;
}

void driver_Draw (struct RastPort * rp, LONG x, LONG y,
		    struct GfxBase * GfxBase)
{
    _aros_not_implemented();
}

ULONG driver_ReadPixel (struct RastPort * rp, LONG x, LONG y,
		    struct GfxBase * GfxBase)
{
    _aros_not_implemented();
    return 0;
}

LONG driver_WritePixel (struct RastPort * rp, LONG x, LONG y,
		    struct GfxBase * GfxBase)
{
    _aros_not_implemented();
    return 0;
}

void driver_PolyDraw (struct RastPort * rp, LONG count, WORD * coords,
		    struct GfxBase * GfxBase)
{
    _aros_not_implemented();
}

void driver_SetRast (struct RastPort * rp, ULONG color,
		    struct GfxBase * GfxBase)
{
    _aros_not_implemented();
}

void driver_SetFont (struct RastPort * rp, struct TextFont * font,
		    struct GfxBase * GfxBase)
{
    _aros_not_implemented();
}

struct TextFont * driver_OpenFont (struct TextAttr * ta,
	struct GfxBase * GfxBase)
{
    _aros_not_implemented();
    return 0;
}

void driver_CloseFont (struct TextFont * tf, struct GfxBase * GfxBase)
{
    _aros_not_implemented();
}

int driver_InitRastPort (struct RastPort * rp, struct GfxBase * GfxBase)
{
    _aros_not_implemented();
    return FALSE;
}

int driver_CloneRastPort (struct RastPort * newRP, struct RastPort * oldRP,
			struct GfxBase * GfxBase)
{
    _aros_not_implemented();
    return FALSE;
}

void driver_DeinitRastPort (struct RastPort * rp, struct GfxBase * GfxBase)
{
    _aros_not_implemented();
}

