#ifndef AMIGAGUIDE_INTERN_H
#define AMIGAGUIDE_INTERN_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal definitions for amigaguide.library.
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

#ifndef LIBRARIES_AMIGAGUIDE_H
#   include <libraries/amigaguide.h>
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
#define SysBase (((struct LibHeader *) AmigaGuideBase)->lh_SysBase)


/*****************
**  Prototypes  **
*****************/


/********************
**  Library stuff  **
********************/


LIBBASETYPE
{
    struct LibHeader lh;
};

#endif /* AMIGAGUIDE_INTERN_H */
