/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open the icon information window for a specified file.
    Lang: english
*/

#include <exec/types.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <workbench/workbench.h>

#include "workbench_intern.h"

#include <aros/debug.h>

/*****************************************************************************

    NAME */
    #include <proto/workbench.h>

    AROS_LH3(ULONG, WBInfo,
/*  SYNOPSIS */
    AROS_LHA(BPTR,            lock,   A0),
    AROS_LHA(STRPTR,          name,   A1),
    AROS_LHA(struct Screen *, screen, A2),

/*  LOCATION */
    struct WorkbenchBase *, WorkbenchBase, 15, Workbench)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct WorkbenchBase *, WorkbenchBase)

    aros_print_not_implemented ("WBInfo");
#warning TODO: Write Workbench/WBInfo

    return NULL;

    AROS_LIBFUNC_EXIT
} /* WBInfo */

