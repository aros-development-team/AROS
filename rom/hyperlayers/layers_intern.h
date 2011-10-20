/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal information for layers.library.
    Lang:
*/
#ifndef _LAYERS_INTERN_H_
#define _LAYERS_INTERN_H_

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/libraries.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <utility/utility.h>
#include <setjmp.h>
#include <dos/dos.h>   /* BPTR below */
#include <proto/alib.h> /* We redefine NewRectRegion() */

#include LC_LIBDEFS_FILE

#include "intregions.h"

LIBBASETYPE
{
    struct Library   	    lb_LibNode;

    APTR    	    	    lb_ClipRectPool;

    struct GfxBase *        lb_GfxBase;
    struct UtilityBase *    lb_UtilityBase;
};

/* Store library bases in our base, not in .bss. Why not ? */
#define GfxBase     (LIBBASE->lb_GfxBase)
#define UtilityBase (LIBBASE->lb_UtilityBase)

struct IntLayer
{
    struct Layer    lay;
    struct Hook	   *shapehook;
    IPTR	    window;	/* This is passed to shape hook. Comes from Intuition. */
    ULONG	    intflags;
    struct RastPort rp;		/* lay.rp */
};

#define IL(x) ((struct IntLayer *)(x))

/* Standard layer priorities */
#define ROOTPRIORITY		0
#define BACKDROPPRIORITY	10
#define UPFRONTPRIORITY		20

#define INTFLAG_AVOID_BACKFILL 1

struct LayerInfo_extra
{
#if 0
    ULONG          lie_ReturnAddr;     // used by setjmp/longjmp, equals jmp_buf
    ULONG          lie_Regs[12];       // D2-D7/A2-SP
#else
    jmp_buf        lie_JumpBuf;
#endif
    struct MinList lie_ResourceList;
    UBYTE          lie_pad[4];
};

/*
 * These are special types of ResData resources. If layers finds one of
 * these values in ResData->Size, it performs some special handling to
 * properly dispose of the allocated Region or BitMap, respectively
 * (throught DisposeRegion or FreeBitMap). In all other cases,
 * ResData->Size is an argument for a freemem operation.
 */
#define RD_REGION -1
#define RD_BITMAP -2

struct ResData
{
    void *ptr;
    ULONG Size;
};

struct ResourceNode
{
    struct Node	    rn_Link;
    struct ResData *rn_FirstFree;
    LONG            rn_FreeCnt;
    struct ResData  rn_Data[48];
};


/*
** The smart refresh flag is set for super bitmap as well as smart refresh
** layers 
*/
#define IS_SIMPLEREFRESH(l) (0 != ((l)->Flags & LAYERSIMPLE))
#define IS_SMARTREFRESH(l)  (LAYERSMART == ((l)->Flags & (LAYERSMART|LAYERSUPER)))
#define IS_SUPERREFRESH(l)  (0 != ((l)->Flags & LAYERSUPER))

int _MoveLayerBehind(struct Layer *l,
                     struct Layer *lfront,
                     LIBBASETYPEPTR LayersBase);
int _MoveLayerToFront(struct Layer * l,
                      struct Layer * lbehind,
                      LIBBASETYPEPTR LayersBase);

#define NewRectRegion(MinX, MinY, MaxX, MaxY) _NewRectRegion(MinX, MinY, MaxX, MaxY, LIBBASE->lb_GfxBase)

#endif /* _LAYERS_INTERN_H */
