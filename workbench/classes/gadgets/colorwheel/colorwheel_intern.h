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

#ifndef AROS_DEBUG_H
#include <aros/debug.h>
#endif

#include "libdefs.h"


/* Predeclaration */
LIBBASETYPE;


/**************
**  Defines  **
**************/
#define SysBase (((struct LibHeader *) ColorWheelBase)->lh_SysBase)

#undef EG
#define EG(o) ((struct ExtGadget *)o)

#define HSPACING	2
#define VSPACING	3

#define HBORDER	HSPACING
#define VBORDER (VSPACING - 1)


#define HSELBORDER	1
#define VSELBORDER	1

struct ColorWheelData
{

    
};


/*****************
**  Prototypes  **
*****************/

struct IClass * InitColorWheelClass (LIBBASETYPEPTR);


/********************
**  Library stuff  **
********************/
/*
extern struct ExecBase *SysBase;
*/

LIBBASETYPE
{
    struct LibHeader lh;
    struct IClass *classptr;
};

#endif /* COLORWHEEL_INTERN_H */
