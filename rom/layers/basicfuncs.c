/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Basic support functions for layers.library.
    Lang: English.
*/

#include <aros/config.h>
#include <aros/asmcall.h>
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <graphics/clip.h>
#include <graphics/regions.h>
#include <graphics/layers.h>
#include <graphics/gfx.h>
#include <utility/hooks.h>
#include <setjmp.h>

#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#include "layers_intern.h"
#include "basicfuncs.h"

#define DEBUG 1
#include <aros/debug.h>
#undef kprintf

/*
 *  Sections:
 *
 *  + Blitter
 *  + Hook
 *  + Layer
 *  + LayerInfo
 *  + Rectangle
 *  + Miscellaneous
 *
 */

/***************************************************************************/
/*                                 BLITTER                                 */
/***************************************************************************/

#define CR2NR_NOBITMAP 0
#define CR2NR_BITMAP   1

#if !(AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
/*
 * These functions cause the infamous "fixed or forbidden register was spilled"
 * bug/feature in m68k gcc, so these were written straight in asm. They can be
 * found in config/m68k-native/layers, for the m68k AROSfA target. Other targets,
 * that use stack passing, can use these versions.
 */

AROS_UFH4(void, _BltRPtoCR,
    AROS_UFHA(struct RastPort *,   rp,         A0),
    AROS_UFHA(struct ClipRect *,   cr,         A1),
    AROS_UFHA(ULONG,               Mode,       D0),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    BltBitMap(rp->BitMap, cr->bounds.MinX, cr->bounds.MinY,
	      cr->BitMap, cr->bounds.MinX & 0xf, 0,
	      cr->bounds.MaxX - cr->bounds.MinX + 1,
	      cr->bounds.MaxY - cr->bounds.MinY + 1,
	      Mode, ~0, NULL);
}

AROS_UFH4(void, _BltCRtoRP,
    AROS_UFHA(struct RastPort *,   rp,         A0),
    AROS_UFHA(struct ClipRect *,   cr,         A1),
    AROS_UFHA(ULONG,               Mode,       D0),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    BltBitMap(cr->BitMap, cr->bounds.MinX & 0xf, 0,
	      rp->BitMap, cr->bounds.MinX, cr->bounds.MinY,
	      cr->bounds.MaxX - cr->bounds.MinX + 1,
	      cr->bounds.MaxY - cr->bounds.MinY + 1,
	      Mode, ~0, NULL);
}

#endif /* if !native */

/***************************************************************************/
/*                                  HOOK                                   */
/***************************************************************************/

/* Will be filled in a bit later... */

/***************************************************************************/
/*                                 LAYER                                   */
/***************************************************************************/

/* Will be filled in a bit later... */

/***************************************************************************/
/*                               LAYERINFO                                 */
/***************************************************************************/

/*-------------------------------------------------------------------------*/
/*
 * Allocate LayerInfo_extra and initialize its list.
 */
AROS_UFH2(BOOL, _AllocExtLayerInfo,
    AROS_UFHA(struct Layer_Info *, li,         A0),
    AROS_UFHA(struct LayersBase *, LayersBase, A6))
{
    if(++li->fatten_count != 0)
	return TRUE;

    if(!(li->LayerInfo_extra = AllocMem(sizeof(struct LayerInfo_extra),MEMF_PUBLIC|MEMF_CLEAR)))
	return FALSE;

    NewList((struct List *)&((struct LayerInfo_extra *)li->LayerInfo_extra)->lie_ResourceList);

    return TRUE;
}

/*
 * Free LayerInfo_extra.
 */
void _FreeExtLayerInfo(struct Layer_Info *li, struct LayersBase *LayersBase)
{
    DB2(bug("FreeExtLayerInfo($%lx)...", li));

    if(--li->fatten_count >= 0)
	return;

    if(li->LayerInfo_extra == NULL)
	return;

    FreeMem(li->LayerInfo_extra, sizeof(struct LayerInfo_extra));

    li->LayerInfo_extra = NULL;

    DB2(bug("done\n"));
}

/***************************************************************************/
/*                                RECTANGLE                                */
/***************************************************************************/

/* Will be filled in a bit later... */

/***************************************************************************/
/*                              MISCELLANEOUS                              */
/***************************************************************************/

/* Will be filled in a bit later... */



/*-----------------------------------END-----------------------------------*/

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

