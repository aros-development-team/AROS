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
    APTR DOSBase;
    IPTR rc = FALSE;
  
    /* Start Workbook, if we have it. */
    D(bug("StartWorkbench: ptr = %p\n", ptr));
    if ((DOSBase = OpenLibrary("dos.library", 0))) {
    	struct Segment *seg;
    	BPTR wbseg = 0;

    	Forbid();
    	seg = FindSegment("Workbook", NULL, TRUE);
    	if (seg != NULL)
    	    wbseg = seg->seg_Seg;
    	Permit();
    	D(bug("StartWorkbench: DOSBase = %p, Segment = %p\n", DOSBase, BADDR(wbseg)));
    	if (wbseg) {
    	    /* This task now becomes Workbench */
    	    ULONG (*wbfunc)(void) = BADDR(wbseg);
    	    rc = wbfunc();
    	}
    }

    return rc;
        
    AROS_LIBFUNC_EXIT
} /* StartWorkbench() */
