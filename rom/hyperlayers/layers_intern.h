/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
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
#include <graphics/rastport.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <setjmp.h>
#include <dos/dos.h>   /* BPTR below */

#include "libdefs.h"


#include "../graphics/intregions.h"

LIBBASETYPE
{
    struct Library   	    lb_LibNode;

    BPTR	     	    lb_SegList;
    
    struct GfxBase 	    *lb_GfxBase;
    struct ExecBase 	    *lb_SysBase;
    struct UtilityBase      *lb_UtilityBase;
    struct SignalSemaphore  lb_MemLock;
    APTR    	    	    lb_ClipRectPool;
};

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


/* digulla again... Needed for close() */
#define expunge() \
 AROS_LC0(BPTR, expunge, LIBBASETYPEPTR, LIBBASE, 3, BASENAME)

#define SysBase         LIBBASE->lb_SysBase
#define GfxBase		LIBBASE->lb_GfxBase
#define UtilityBase	LIBBASE->lb_UtilityBase

/*
** The smart refresh flag is set for super bitmap as well as smart refresh
** layers 
*/
#define IS_SIMPLEREFRESH(l) (0 != ((l)->Flags & LAYERSIMPLE))
#define IS_SMARTREFRESH(l)  (LAYERSMART == ((l)->Flags & (LAYERSMART|LAYERSUPER)))
#define IS_SUPERREFRESH(l)  (0 != ((l)->Flags & LAYERSUPER))

#warning Might want to move this to a public include file.
struct ChangeLayerShapeMsg
{
  struct Region   * newshape; // same as passed to ChangeLayerShape()
  struct ClipRect * cliprect;
  struct Region   * shape;
};

int _MoveLayerBehind(struct Layer *l,
                     struct Layer *lfront,
                     struct LayersBase * LayersBase);
int _MoveLayerToFront(struct Layer * l,
                      struct Layer * lbehind,
                      struct LayersBase * LayersBase);


#endif /* _LAYERS_INTERN_H */
