/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

#include "libdefs.h"

/* Can these be safely removed ... */
#include <aros/libcall.h>
#include <dos/dos.h>
#include <graphics/gfxbase.h>
/* ... ??? */

extern struct GfxBase * GfxBase;

struct LIBBASETYPE
{
    struct Library   	lb_LibNode;

    BPTR	     	lb_SegList;
    
    struct Library  	*lb_GfxBase;
    struct Library  	*lb_UtilityBase;
    struct ExecBase 	*lb_SysBase;
};

struct LayerInfo_extra
{
    struct MinList 	lie_ResourceList;
};

struct IntLayer
{
    struct Layer     	lay;
    struct Region    	*shape;
    struct Region	*visiblearea;
    struct Region	*hiddenarea;
    ULONG	     	intflags;
    WORD	     	pri;
};

#define IL(x) 			((struct IntLayer *)x)

#define LFLG_INVISIBLE 		1

#define IS_SIMPLELAYERFLAG(x) 	((x) & LAYERSIMPLE)
#define IS_SMARTLAYERFLAG(x)  	(((x) & (LAYERSMART | LAYERSUPER)) == LAYERSMART)
#define IS_SUPERLAYERFLAG(x)  	(((x) & (LAYERSMART | LAYERSUPER)) == LAYERSUPER)

#define IS_SIMPLELAYER(x) 	IS_SIMPLELAYERFLAG((x)->Flags)
#define IS_SMARTLAYER(x)  	IS_SMARTLAYERFLAG((x)->Flags)
#define IS_SUPERLAYER(x)  	IS_SUPERLAYERFLAG((x)->Flags)

#define IS_BACKDROPFLAG(x) 	((x) & LAYERBACKDROP)

#define IS_BACKDROP(x) 		IS_BACKDROPFLAG((x)->Flags)

#define IS_INVISIBLE(x) 	(IL(x)->intflags & LFLG_INVISIBLE)

#define LAYERTYPE_TO_FLAG(x)	(((x) == LAYERSIMPLE) ? LAYERSIMPLE : \
			        (((x) == LAYERSMART) ? LAYERSMART : LAYERSMART | LAYERSUPER))

#define LAYERPRI_TO_FLAG(x) 	(((x) == LPRI_BACKDROP) ? LAYERBACKDROP : 0)

#define DO_INTERSECT(lay1,lay2) ( ( ((lay1)->bounds.MinX > (lay2)->bounds.MaxX) || \
	    			((lay1)->bounds.MaxX < (lay2)->bounds.MinX) || \
	    			((lay1)->bounds.MinY > (lay2)->bounds.MaxY) || \
	    			((lay1)->bounds.MaxY < (lay2)->bounds.MinY) ) ? FALSE : TRUE )
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
    void 		*ptr;
    ULONG 		Size;
};

struct ResourceNode
{
    struct Node	    	rn_Link;
    struct ResData 	*rn_FirstFree;
    LONG            	rn_FreeCnt;
    struct ResData  	rn_Data[48];
};


/* digulla again... Needed for close() */
#define expunge() \
 AROS_LC0(BPTR, expunge, struct LIBBASETYPE *, LIBBASE, 3, BASENAME)

/*
#define SysBase         LIBBASE->lb_SysBase
#define GfxBase		LIBBASE->lb_GfxBase
*/

#endif /* _LAYERS_INTERN_H */
