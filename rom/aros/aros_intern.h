/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal information for aros.library.
    Lang:
*/
#ifndef _AROS_INTERN_H_
#define _AROS_INTERN_H_

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dos.h>

#include LC_LIBDEFS_FILE

/*
    This is the ArosBase structure. It is documented here because it is
    completely private. Applications should treat it as a struct Library, and
    use the aros.library functions to get information.
*/

LIBBASETYPE
{
    struct Library       aros_LibNode;

    /* The following information is private! */

    struct ExecBase *    aros_sysBase;
    struct Library *     aros_utilityBase;
    BPTR                 aros_segList;

};

#define SysBase         LIBBASE->aros_sysBase
#define UtilityBase	LIBBASE->aros_utilityBase

#endif /* _AROS_INTERN_H */
