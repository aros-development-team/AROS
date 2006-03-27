/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
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

#include LC_LIBDEFS_FILE


#include "../graphics/intregions.h"

LIBBASETYPE
{
    struct Library   	    lb_LibNode;

    BPTR	     	    lb_SegList;
    
    struct ExecBase 	    *lb_SysBase;
    struct SignalSemaphore  lb_MemLock;
    APTR    	    	    lb_ClipRectPool;
};

struct IntLayer
{
    struct Layer lay;
    struct Hook  *shapehook;
    ULONG   	 intflags;
};

#define IL(x) ((struct IntLayer *)(x))

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


#endif /* _LAYERS_INTERN_H */
