#ifndef GRADIENTSLIDER_INTERN_H
#define GRADIENTSLIDER_INTERN_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal definitions for gradientslider.gadget.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef LIBCORE_BASE_H
#   include <libcore/base.h>
#endif
#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif

#ifdef _AROS
#ifndef AROS_DEBUG_H
#include <aros/debug.h>
#endif
#endif

#include "libdefs.h"

/***************************************************************************************************/

#ifndef _AROS
#define DeinitRastPort(x)
#define EnterFunc(x)
#define bug
#define D(x)
#define ReturnPtr(a,b,c) return c
#define ReturnInt(a,b,c) return c
#define ReturnVoid(a) return
#endif

#define SysBase (((struct LibHeader *) GradientSliderBase)->lh_SysBase)

#undef 	EG
#define EG(o) ((struct ExtGadget *)o)

#define FRAMESLIDERSPACINGX 3
#define FRAMESLIDERSPACINGY 3

/***************************************************************************************************/

struct GradientSliderData
{
    struct BitMap		*savebm;
    struct BitMap	    	*knobbm;
    Object			*frame;
    UBYTE			*buffer;
    ULONG			maxval;		/* ISGU 	*/
    ULONG			curval;		/* ISGNU 	*/
    ULONG			saveval;
    ULONG			skipval;	/* ISGU		*/
    UWORD			knobpixels;	/* I		*/
    UWORD			*penarray;	/* ISU		*/
    WORD			freedom;	/* I		*/
    WORD			numpens;
    WORD			clickoffsetx;
    WORD			clickoffsety;
    WORD			savefromx;
    WORD			savefromy;
    WORD			savebmwidth;
    WORD			savebmheight;
    WORD			x,y;
    struct RastPort	    	trp;
};


struct GradientSliderBase_intern
{
    struct Library 		library;
    UWORD   	    	    	pad;
    struct IClass 		*classptr;
    struct ExecBase		*sysbase;
    BPTR			seglist;
#ifndef GLOBAL_INTUIBASE
    struct IntuitionBase	*intuibase;
#endif
    struct GfxBase		*gfxbase;
    struct Library		*utilitybase;
    struct Library		*cybergfxbase;
    struct Library  	    	*layersbase;
    
};

/***************************************************************************************************/

struct IClass * InitGradientSliderClass (struct GradientSliderBase_intern *GradientSliderBase);
VOID DrawGradient(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2, UWORD *penarray,
		  WORD numpens, WORD orientation, struct ColorMap *cm, 
		  struct GradientSliderBase_intern *GradientSliderBase);
VOID DrawKnob(struct GradientSliderData *data, struct RastPort *rp, struct DrawInfo *dri, 
	      struct IBox *box, WORD state, struct GradientSliderBase_intern *GradientSliderBase);
VOID GetGadgetIBox(Object *o, struct GadgetInfo *gi, struct IBox *ibox);
VOID GetSliderBox(struct IBox *gadgetbox, struct IBox *sliderbox);
VOID GetKnobBox(struct GradientSliderData *data, struct IBox *sliderbox, struct IBox * knobbox);
void DrawDisabledPattern(struct RastPort *rport, struct IBox *gadbox, UWORD pen,
			 struct GradientSliderBase_intern *GradientSliderBase);


/***************************************************************************************************/

/* The following typedefs are necessary, because the names of the global
   variables storing the library base pointers	and the corresponding
   structs are equal.
   This is a hack, of course. */
typedef struct GfxBase GraphicsBase;
typedef struct IntuitionBase IntuiBase;

/***************************************************************************************************/

#undef GSB
#define GSB(b) ((struct GradientSliderBase_intern *)b)
#undef UtilityBase
#define UtilityBase 	GSB(GradientSliderBase)->utilitybase


#ifndef GLOBAL_INTUIBASE
#undef IntuitionBase
#define IntuitionBase	GSB(GradientSliderBase)->intuibase
#endif

#define CyberGfxBase	GSB(GradientSliderBase)->cybergfxbase

#undef GfxBase
#define GfxBase		GSB(GradientSliderBase)->gfxbase
#undef SysBase
#define SysBase		GSB(GradientSliderBase)->sysbase

#undef LayersBase
#define LayersBase		GSB(GradientSliderBase)->layersbase

#endif /* GRADIENTSLIDER_INTERN_H */
