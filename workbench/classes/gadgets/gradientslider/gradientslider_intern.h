#ifndef GRADIENTSLIDER_INTERN_H
#define GRADIENTSLIDER_INTERN_H

/*
    (C) 2000 AROS - The Amiga Research OS
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

#ifndef AROS_DEBUG_H
#include <aros/debug.h>
#endif

#include "libdefs.h"

/***************************************************************************************************/

#define SysBase (((struct LibHeader *) GradientSliderBase)->lh_SysBase)

#undef 	EG
#define EG(o) ((struct ExtGadget *)o)

/***************************************************************************************************/

struct GradientSliderData
{
    WORD 			dummy;
};


struct GradientSliderBase_intern
{
    struct Library 		library;
    struct ExecBase		*sysbase;
    BPTR			seglist;
    struct IClass 		*classptr;
#ifndef GLOBAL_INTUIBASE
    struct IntuitionBase	*intuibase;
#endif
    struct GfxBase		*gfxbase;
    struct Library		*utilitybase;
    
};

/***************************************************************************************************/

struct IClass * InitGradientSliderClass (struct GradientSliderBase_intern *GradientSliderBase);
VOID GetGadgetIBox(Object *o, struct GadgetInfo *gi, struct IBox *ibox);
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

#undef PB
#define PB(b) ((struct GradientSliderBase_intern *)b)
#undef UtilityBase
#define UtilityBase 	PB(GradientSliderBase)->utilitybase


#ifndef GLOBAL_INTUIBASE
#undef IntuitionBase
#define IntuitionBase	PB(GradientSliderBase)->intuibase
#endif

#undef GfxBase
#define GfxBase		PB(GradientSliderBase)->gfxbase
#undef SysBase
#define SysBase		PB(GradientSliderBase)->sysbase

#endif /* GRADIENTSLIDER_INTERN_H */
