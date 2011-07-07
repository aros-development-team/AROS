#ifndef GRAPHICS_CLIP_H
#define GRAPHICS_CLIP_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
    struct Layer     	    * front;
    struct Layer     	    * back;
    struct ClipRect  	    * ClipRect;
    struct RastPort  	    * rp;
    struct Rectangle          bounds;

#if 1
    struct Layer     	    * parent; 	    	    	/* PRIVATE !!! */
#else
    UBYTE   	    	      reserved[4];
#endif
    UWORD   	    	      priority;
    UWORD   	    	      Flags;

    struct BitMap   	    * SuperBitMap;
    struct ClipRect 	    * SuperClipRect;

    APTR    	    	      Window;
    WORD    	    	      Scroll_X;
    WORD    	    	      Scroll_Y;

    struct ClipRect 	    * cr;
    struct ClipRect 	    * cr2;
    struct ClipRect 	    * crnew;
    struct ClipRect 	    * SuperSaveClipRects;
    struct ClipRect 	    * _cliprects;

    struct Layer_Info       * LayerInfo;
    struct SignalSemaphore    Lock;
    struct Hook             * BackFill;

#if 1
    struct Region   	    * VisibleRegion; 	    	/* PRIVATE !!! */
#else
    ULONG   	    	      reserved1;
#endif

    struct Region   	    * ClipRegion;
    struct Region   	    * saveClipRects;

    WORD    	    	      Width;
    WORD    	    	      Height;

#if 1
    struct Region   	    * shape;	    	    	/* PRIVATE !!! */
    struct Region   	    * shaperegion;  	    	/* PRIVATE !!! */
    struct Region   	    * visibleshape; 	    	/* PRIVATE !!! */

    UWORD   	    	      nesting;	    	    	/* PRIVATE !!! */
    UBYTE   	    	      SuperSaveClipRectCounter;	/* PRIVATE !!! */
    UBYTE   	    	      visible;	    	    	/* PRIVATE !!! */

    UBYTE   	    	      reserved2[2]; 
#else
    UBYTE   	    	      reserved2[18];
#endif

    struct Region   	    * DamageList;
};

#define MAXSUPERSAVECLIPRECTS	20	/* Max. number of cliprects that are kept preallocated in the list */

struct ClipRect
{
    struct ClipRect  	    * Next;
    struct ClipRect  	    * prev;
    struct Layer     	    * lobs;
    struct BitMap    	    * BitMap;
    struct Rectangle          bounds;

    void    	    	    * _p1;
    void    	    	    * _p2;
    LONG    	    	      reserved;
    LONG    	    	      Flags;
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
#define LA_Visible	WA_Hidden
#define LA_Shape	WA_ShapeRegion
#define LA_ShapeHook	WA_ShapeHook


/*
 * Tags for scale layer
 */
#define LA_SRCX	      0x4000
#define LA_SRCY       0x4001
#define LA_DESTX      0x4002
#define LA_DESTY      0x4003
#define LA_SRCWIDTH   0x4004
#define LA_SRCHEIGHT  0x4005
#define LA_DESTWIDTH  0x4006
#define LA_DESTHEIGHT 0x4007


#define ROOTPRIORITY		0
#define BACKDROPPRIORITY	10
#define UPFRONTPRIORITY		20

#define IS_VISIBLE(l) (TRUE == l->visible)

struct ChangeLayerShapeMsg
{
  struct Region   * newshape; // same as passed to ChangeLayerShape()
  struct ClipRect * cliprect;
  struct Region   * shape;
};

struct CollectPixelsLayerMsg
{
  LONG   xSrc;
  LONG   ySrc;
  LONG   width;
  LONG   height;
  LONG   xDest;
  LONG   yDest;
  struct BitMap * bm;
  struct Layer *  layer;
  ULONG  minterm;
};

/* Msg sent through LA_ShapeHook. Hook function must look like this:

    AROS_UFH3(struct Region *, MyShapeFunc,
    	AROS_UFHA(struct Hook *, hook, A0),
    	AROS_UFHA(struct Layer *, layer, A2),
    	AROS_UFHA(struct ShapeHookMsg *, msg, A1))

*/

#define SHAPEHOOKACTION_CREATELAYER     0
#define SHAPEHOOKACTION_MOVELAYER	    1
#define SHAPEHOOKACTION_SIZELAYER	    2
#define SHAPEHOOKACTION_MOVESIZELAYER   3

struct ShapeHookMsg
{
    LONG    	     Action;
    struct Layer    *Layer;
    struct Region   *ActualShape;
    struct Rectangle NewBounds;
    struct Rectangle OldBounds;
};

#endif /* GRAPHICS_CLIP_H */

