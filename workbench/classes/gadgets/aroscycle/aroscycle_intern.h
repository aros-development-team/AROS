/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
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

#define TURN_OFF_DEBUG


/* Support */
#define G(obj) ((struct Gadget *)(obj))

/***********************************************************************************/

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

void drawdisabledpattern(struct RastPort *rport, UWORD pen, WORD left, WORD top, UWORD width, UWORD height);
void renderlabel (struct Gadget *gad, STRPTR label, struct RastPort *rport, struct GadgetInfo *ginfo);
BOOL pointingadget(struct Gadget *gad, struct GadgetInfo *gi, WORD x, WORD y);

/***********************************************************************************/

#endif /* AROSCYCLE_INTERN_H */

/***********************************************************************************/
