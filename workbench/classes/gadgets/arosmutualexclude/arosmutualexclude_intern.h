/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
*/

/***********************************************************************************/

#ifndef AROSMUTUALEXCLUDE_INTERN_H
#define AROSMUTUALEXCLUDE_INTERN_H

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

/* MutualExcludeClass definitions */
struct MXData
{
    /* Important pointers */
    struct DrawInfo 	*dri;
    struct TextAttr 	*tattr;
    struct Image    	*mximage;
    struct TextFont 	*font;
    struct Rectangle	bbox;
    /* Information about ticks */
    ULONG   	    	active, newactive; /* The active tick and the tick to be activated */

    /* Information about labels */
    STRPTR  	    	*labels;
    ULONG   	    	numlabels; /* The number of labels */
    LONG    	    	labelplace, ticklabelplace;
    UWORD   	    	fontheight;
    UWORD   	    	spacing;
    UWORD   	    	maxtextwidth;
};

/***********************************************************************************/

/* Prototypes */

void drawdisabledpattern(struct RastPort *rport,
			 UWORD pen,
			 WORD left, WORD top, UWORD width, UWORD height
);
BOOL renderlabel(struct Gadget *gad, struct RastPort *rport, struct MXData *data);


/***********************************************************************************/

#endif /* AROSMUTUALEXCLUDE_INTERN_H */

/***********************************************************************************/
