#ifndef COLORWHEEL_INTERN_H
#define COLORWHEEL_INTERN_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal definitions for colorwheel.gadget.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifdef __AROS__
#ifndef LIBCORE_BASE_H
#   include <libcore/base.h>
#endif
#endif
#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef GADGETS_COLORWHEEL_H
#   include <gadgets/colorwheel.h>
#endif
#ifndef DOS_DOS_H
#	include <dos/dos.h>
#endif

#ifdef __AROS__
#ifndef AROS_DEBUG_H
#include <aros/debug.h>
#endif
#endif

#define FIXED_MATH		0
//#define USE_ALLOCRASTER

#if FIXED_MATH
#include "fixmath.h"
#endif

#include "libdefs.h"

/***************************************************************************************************/

#ifndef __AROS__
#define DeinitRastPort(x)
#define EnterFunc(x)
#define bug
#define D(x)
#define ReturnPtr(a,b,c) return c
#define ReturnInt(a,b,c) return c
#define ReturnVoid(a) return
#endif

#define SysBase 		(((struct LibHeader *) ColorWheelBase)->lh_SysBase)

#undef 	EG
#define EG(o) 			((struct ExtGadget *)o)

#define KNOBWIDTH		7
#define KNOBHEIGHT		7
#define KNOBCX			3
#define KNOBCY			3
	
#define BORDERWHEELSPACINGX 	4
#define BORDERWHEELSPACINGY 	4

/***************************************************************************************************/

struct ColorWheelData
{
    struct ColorWheelHSB	hsb;			/* ISGNU	*/
    struct ColorWheelRGB	rgb;			/* ISGNU	*/ 
    struct Screen		*scr;			/* I 		*/
    Object			*gradobj;		/* IS 		*/
    STRPTR			abbrv;			/* I 		*/
    UWORD			*donation;		/* I 		*/
    UWORD			maxpens;		/* I 		*/
    
    struct DrawInfo		*dri;
    struct BitMap		*bm;
    PLANEPTR			 mask;
    struct BitMap		*savebm;
    struct RastPort		 trp;
    Object			*frame;
    LONG			*rgblinebuffer;
    struct Hook     	    	*backfill;
    WORD 			rgblinebuffer_size;
    WORD			bmwidth;
    WORD			bmheight;
    WORD			wheelcx;
    WORD			wheelcy;
    WORD			wheelrx;
    WORD			wheelry;
    WORD			knobsavex;
    WORD			knobsavey;
    BOOL			wheeldrawn;
    UWORD			range;
    UWORD			levels;
    WORD    	    	    	pens[6*6*6];
    BOOL    	    	    	gotpens;
};

struct ColorWheelBase_intern
{
    struct Library 		library;
    struct ExecBase		*sysbase;
    BPTR			seglist;
    struct IClass 		*classptr;
#ifndef GLOBAL_INTUIBASE
    struct IntuitionBase	*intuibase;
#endif
    struct GfxBase		*gfxbase;
    struct Library		*cybergfxbase;
    struct Library		*utilitybase;
    struct Library		*layersbase;    
};

/***************************************************************************************************/

struct IClass *InitColorWheelClass (struct ColorWheelBase_intern *ColorWheelBase);

#if FIXED_MATH
BOOL CalcWheelColor(LONG x, LONG y, LONG cx, LONG cy, ULONG *hue, ULONG *sat);
#else
BOOL CalcWheelColor(LONG x, LONG y, double cx, double cy, ULONG *hue, ULONG *sat);
#endif
VOID RenderWheel(struct ColorWheelData *data, struct RastPort *rp, struct IBox *box,
		 struct ColorWheelBase_intern *ColorWheelBase);
VOID RenderKnob(struct ColorWheelData *data, struct RastPort *rp, struct IBox *gbox, BOOL update,
		struct ColorWheelBase_intern *ColorWheelBase);
VOID GetGadgetIBox(Object *o, struct GadgetInfo *gi, struct IBox *ibox);
void DrawDisabledPattern(struct ColorWheelData *data, struct RastPort *rport, struct IBox *gadbox,
			 struct ColorWheelBase_intern *ColorWheelBase);
void allocPens( struct ColorWheelData *data, struct ColorWheelBase_intern *ColorWheelBase );
void freePens( struct ColorWheelData *data, struct ColorWheelBase_intern *ColorWheelBase );

/***************************************************************************************************/

/* The following typedefs are necessary, because the names of the global
   variables storing the library base pointers	and the corresponding
   structs are equal.
   This is a hack, of course. */
   
typedef struct GfxBase GraphicsBase;
typedef struct IntuitionBase IntuiBase;

/***************************************************************************************************/

#undef CWB
#define CWB(b) 			((struct ColorWheelBase_intern *)b)
#undef UtilityBase
#define UtilityBase 		CWB(ColorWheelBase)->utilitybase


#ifndef GLOBAL_INTUIBASE
#undef IntuitionBase
#define IntuitionBase		CWB(ColorWheelBase)->intuibase
#endif

#undef GfxBase
#define GfxBase			CWB(ColorWheelBase)->gfxbase

#define CyberGfxBase    	CWB(ColorWheelBase)->cybergfxbase

#undef SysBase
#define SysBase			CWB(ColorWheelBase)->sysbase

#undef LayersBase
#define LayersBase		CWB(ColorWheelBase)->layersbase

#endif /* COLORWHEEL_INTERN_H */
