#ifndef GRAPHICS_CLIP_H
#define GRAPHICS_CLIP_H

/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Clip descriptions.
    Lang: english
*/

#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef GRAPHICS_GFX_H
#   include <graphics/gfx.h>
#endif
#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif

#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif

#define NEWLOCKS


struct Layer
{
    struct Layer     * front;
    struct Layer     * back;
    struct ClipRect  * ClipRect;
    struct RastPort  * rp;
    struct Rectangle   bounds;

#if 1
    struct Layer * parent;
#else
    UBYTE reserved[4];
#endif
    UWORD priority;
    UWORD Flags;

    struct BitMap   * SuperBitMap;
    struct ClipRect * SuperClipRect;

    APTR Window;
    WORD Scroll_X;
    WORD Scroll_Y;

    struct ClipRect * cr;
    struct ClipRect * cr2;
    struct ClipRect * crnew;
    struct ClipRect * SuperSaveClipRects;
    struct ClipRect * _cliprects;

    struct Layer_Info      * LayerInfo;
    struct SignalSemaphore   Lock;
    struct Hook            * BackFill;

#if 1
    struct Region * VisibleRegion;
    struct Region * shape;
    struct Region * shaperegion;
    unsigned int nesting;
#else
    ULONG reserved1;
#endif

    struct Region * ClipRegion;
    struct Region * saveClipRects;

    WORD Width;
    WORD Height;

    UBYTE SuperSaveClipRectCounter;
#if 1
    UBYTE visible;
    UBYTE reserved2[16];
#else
    UBYTE reserved2[17];
#endif

    struct Region * DamageList;
};

#define MAXSUPERSAVECLIPRECTS	20	/* Max. number of cliprects that are kept preallocated in the list */

struct ClipRect
{
    struct ClipRect  * Next;
    struct ClipRect  * prev;
    struct Layer     * lobs;
    struct BitMap    * BitMap;
    struct Rectangle   bounds;

    void * _p1;
    void * _p2;
    LONG   reserved;
    LONG   Flags;
};

/* PRIVATE */
#define CR_NEEDS_NO_CONCEALED_RASTERS 1
#define CR_NEEDS_NO_LAYERBLIT_DAMAGE  2

#define ISLESSX (1<<0)
#define ISLESSY (1<<1)
#define ISGRTRX (1<<2)
#define ISGRTRY (1<<3)

/* This one is used for determining optimal offset for blitting into
cliprects */
#define ALIGN_OFFSET(x) ((x) & 0x0F)


#define LA_Priority	WA_Priority
#define LA_Hook		WA_BackFill
#define LA_SuperBitMap	WA_SuperBitMap
#define LA_ChildOf	WA_Parent
#define LA_InFrontOf	WA_InFrontOf
#define LA_Behind	WA_Behind
#define LA_Visible	WA_Visible
#define LA_Shape	WA_Shape

#define ROOTPRIORITY		0
#define BACKDROPPRIORITY	10
#define UPFRONTPRIORITY		20

#define IS_VISIBLE(l) (TRUE == l->visible)


#endif /* GRAPHICS_CLIP_H */

