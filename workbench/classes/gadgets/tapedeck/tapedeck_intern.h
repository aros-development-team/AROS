/*
    Copyright © 2016, The AROS Development Team. All rights reserved.
    $Id$
*/

/***********************************************************************************/

#ifndef TAPEDECK_INTERN_H
#define TAPEDECK_INTERN_H

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
struct TapeDeckData
{
    struct Gadget               *tdd_PosProp;
    ULONG                       tdd_Mode;
    ULONG                       tdd_FrameCount;
    ULONG                       tdd_FrameCurrent;
};

#endif /* TAPEDECK_INTERN_H */

/***********************************************************************************/
