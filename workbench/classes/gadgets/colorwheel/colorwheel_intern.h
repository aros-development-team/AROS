#ifndef COLORWHEEL_INTERN_H
#define COLORWHEEL_INTERN_H

/*
    (C) 2000 AROS - The Amiga Research OS
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
#ifndef LIBCORE_BASE_H
#   include <libcore/base.h>
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

#ifndef AROS_DEBUG_H
#include <aros/debug.h>
#endif

#include "libdefs.h"

/***************************************************************************************************/

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
    Object			*frame;
    LONG			*rgblinebuffer;
    WORD 			rgblinebuffer_size;
    WORD			bmwidth;
    WORD			bmheight;
    WORD			wheelcx;
    WORD			wheelcy;
    WORD			wheelrx;
    WORD			wheelry;
    WORD			knobsavex;
    WORD			knobsavey;
    BYTE			wheeldrawn;
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
    
};

/***************************************************************************************************/

struct IClass *InitColorWheelClass (struct ColorWheelBase_intern *ColorWheelBase);

BOOL CalcWheelColor(LONG x, LONG y, DOUBLE cx, DOUBLE cy, ULONG *hue, ULONG *sat);
VOID RenderWheel(struct ColorWheelData *data, struct RastPort *rp, struct IBox *box,
		 struct ColorWheelBase_intern *ColorWheelBase);
VOID RenderKnob(struct ColorWheelData *data, struct RastPort *rp, struct IBox *gbox, BOOL update,
		struct ColorWheelBase_intern *ColorWheelBase);
VOID GetGadgetIBox(Object *o, struct GadgetInfo *gi, struct IBox *ibox);
void DrawDisabledPattern(struct RastPort *rport, struct IBox *gadbox, UWORD pen,
			 struct ColorWheelBase_intern *ColorWheelBase);


/***************************************************************************************************/

/* The following typedefs are necessary, because the names of the global
   variables storing the library base pointers	and the corresponding
   structs are equal.
   This is a hack, of course. */
   
typedef struct GfxBase GraphicsBase;
typedef struct IntuitionBase IntuiBase;

/***************************************************************************************************/

#undef CWB
#define CWB(b) 		((struct ColorWheelBase_intern *)b)
#undef UtilityBase
#define UtilityBase 	CWB(ColorWheelBase)->utilitybase


#ifndef GLOBAL_INTUIBASE
#undef IntuitionBase
#define IntuitionBase	CWB(ColorWheelBase)->intuibase
#endif

#undef GfxBase
#define GfxBase		CWB(ColorWheelBase)->gfxbase

#define CyberGfxBase    CWB(ColorWheelBase)->cybergfxbase

#undef SysBase
#define SysBase		CWB(ColorWheelBase)->sysbase

#endif /* COLORWHEEL_INTERN_H */
