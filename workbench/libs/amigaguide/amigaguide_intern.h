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

#ifndef LIBRARIES_AMIGAGUIDE_H
#   include <libraries/amigaguide.h>
#endif

#ifndef AROS_DEBUG_H
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE


/* Predeclaration */


/**************
**  Defines  **
**************/
#define SysBase GM_SYSBASE_FIELD(((LIBBASETYPEPTR)AmigaGuideBase))


/*****************
**  Prototypes  **
*****************/


/********************
**  Library stuff  **
********************/

#endif /* AMIGAGUIDE_INTERN_H */
