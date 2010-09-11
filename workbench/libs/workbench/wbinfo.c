/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Open the file information window for a specified file.
*/

#define DEBUG 0

#include <exec/types.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <workbench/workbench.h>

#include "workbench_intern.h"
#include "support.h"

#include <aros/debug.h>

/*****************************************************************************

    NAME */
    #include <proto/workbench.h>

    AROS_LH3(BOOL, WBInfo,
/*  SYNOPSIS */
    AROS_LHA(BPTR,            lock,   A0),
    AROS_LHA(CONST_STRPTR,    name,   A1),
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
    
    // FIXME: screen argument is ignored
    D(bug("WBInfo('%s', 0x%08lX)\n", name, lock));
    return OpenWorkbenchObject
    (
	name[0] ? "WANDERER:Tools/Info" : "WANDERER:Tools/DiskInfo",
        WBOPENA_ArgLock, (IPTR) lock,
        WBOPENA_ArgName, (IPTR) name,
        TAG_DONE
    );
        
    AROS_LIBFUNC_EXIT
} /* WBInfo() */
