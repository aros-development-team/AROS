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

    AROS_LH2(BOOL, StartWorkbench,
/*  SYNOPSIS */
    AROS_LHA(ULONG,           flags,  D0),
    AROS_LHA(APTR,            ptr,    D1),

/*  LOCATION */
    struct WorkbenchBase *, WorkbenchBase, 7, Workbench)

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
    D(bug("StartWorkbench(0x%08x, @%p)\n", flags, ptr));

    // FIXME: Replace with a call to Wanderer Lite...
    return FALSE;
        
    AROS_LIBFUNC_EXIT
} /* StartWorkbench() */
