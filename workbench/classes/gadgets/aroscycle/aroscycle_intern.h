/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/***********************************************************************************/

#ifndef AROSCYCLE_INTERN_H
#define AROSCYCLE_INTERN_H

/***********************************************************************************/



#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif
#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif
#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef INTUITION_CGHOOKS_H
#   include <intuition/cghooks.h>
#endif
#ifndef INTUITION_GADGETCLASS_H
#   include <intuition/gadgetclass.h>
#endif

/***********************************************************************************/

#define GLOBAL_INTUIBASE

#define TURN_OFF_DEBUG


/* Support */
#define G(obj) ((struct Gadget *)(obj))

/***********************************************************************************/

/* Predeclaration */
struct CycleBase_intern;

/* CycleClass definitions */
struct CycleData
{
    STRPTR  	    *labels;
    struct TextFont *font;
    UWORD   	    active;
    UWORD   	    numlabels;
};

/***********************************************************************************/

/* Prototypes */

void drawdisabledpattern(struct CycleBase_intern *AROSCycleBase, struct RastPort *rport, UWORD pen, WORD left, WORD top, UWORD width, UWORD height);
void renderlabel (struct CycleBase_intern *AROSCycleBase, struct Gadget *gad, STRPTR label, struct RastPort *rport, struct GadgetInfo *ginfo);
BOOL pointingadget(struct Gadget *gad, struct GadgetInfo *gi, WORD x, WORD y);

/***********************************************************************************/

/* Library stuff */
struct CycleBase_intern
{
    struct Library 	    library;
    struct ExecBase	    *sysbase;
    BPTR		    seglist;
    struct Library	    *dosbase;

    #ifndef GLOBAL_INTUIBASE
    struct IntuitionBase    *intuitionbase;
    #endif
    struct GfxBase	    *gfxbase;
    struct Library	    *utilitybase;
    
    struct IClass	    *classptr;
	
};

/***********************************************************************************/

/* The following typedefs are necessary, because the names of the global
   variables storing the library base pointers	and the corresponding
   structs are equal.
   This is a hack, of course. */
   
typedef struct GfxBase GraphicsBase;
typedef struct IntuitionBase IntuiBase;

#undef CYB
#define CYB(b) ((struct CycleBase_intern *)b)
#undef UtilityBase
#define UtilityBase 	CYB(AROSCycleBase)->utilitybase


#ifndef GLOBAL_INTUIBASE
#undef IntuitionBase
#define IntuitionBase	CYB(AROSCycleBase)->intuitionbase
#endif

#undef GfxBase
#define GfxBase		CYB(AROSCycleBase)->gfxbase
#undef SysBase
#define SysBase		CYB(AROSCycleBase)->sysbase
#undef DOSBase
#define DOSBase		CYB(AROSCycleBase)->dosbase


#define expunge() \
AROS_LC0(BPTR, expunge, struct CycleBase_intern *, AROSCycleBase, 3, AROSCycle)

/***********************************************************************************/

#endif /* AROSCYCLE_INTERN_H */

/***********************************************************************************/
