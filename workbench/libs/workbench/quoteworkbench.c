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

    AROS_LH1(BOOL, QuoteWorkbench,
/*  SYNOPSIS */
    AROS_LHA(ULONG,       stringNum,  D0),

/*  LOCATION */
    struct WorkbenchBase *, WorkbenchBase, 6, Workbench)

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
    
    D(bug("QuoteWorkbench(0x%08x)\n", stringNum));
    return TRUE;
        
    AROS_LIBFUNC_EXIT
} /* QuoteWorkbench() */
