/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Open the file information window for a specified file.
*/

#define DEBUG 1

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

    AROS_LH3(BOOL, UpdateWorkbench,
/*  SYNOPSIS */
    AROS_LHA(CONST_STRPTR,    name,   A0),
    AROS_LHA(BPTR,            lock,   A1),
    AROS_LHA(ULONG,           flags,  D0),

/*  LOCATION */
    struct WorkbenchBase *, WorkbenchBase, 5, Workbench)

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
    
    D(bug("UpdateWorkbench(\"%s\", LOCK %p, 0x%08x)\n", name, lock, flags));
    return TRUE;
        
    AROS_LIBFUNC_EXIT
} /* UpdateWorkbench() */
