/*
    (C) 1995-96 AROS - The Amiga Replacement OS
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

#include "libdefs.h"

/* Can these be safely removed ... */
#include <aros/libcall.h>
#include <dos/dos.h>
#include <graphics/gfxbase.h>
/* ... ??? */

extern struct GfxBase * GfxBase;

struct LIBBASETYPE
{
    struct Library   lb_LibNode;

    struct Library * lb_GfxBase;
    struct ExecBase *lb_SysBase;
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

struct InternalClipRect
{
    struct ClipRect  *Next;
    struct ClipRect  *prev;
    struct Layer     *lobs;		/* TRUE if the ClipRect has a BitMap */
    struct BitMap    *BitMap;
    struct Rectangle  bounds;
    union cr_u
    {
	struct Rectangle r;		/* copy of Bounds */
	struct cr_s
	{
	    struct ClipRect *NextCR;
	    struct Layer    *_p2;
	};
    };
    LONG	      reserved;
    LONG	      Flags;
};

/* digulla again... Needed for close() */
#define expunge() \
 AROS_LC0(BPTR, expunge, struct LIBBASETYPE *, LIBBASE, 3, BASENAME)

#define SysBase         LIBBASE->lb_SysBase
#define GfxBase		LIBBASE->lb_GfxBase

#endif /* _LAYERS_INTERN_H */
