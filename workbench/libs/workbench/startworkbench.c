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
    IPTR rc = FALSE;
  
    /* Start Workbook, if we have it. */
    D(bug("StartWorkbench: ptr = %p\n", ptr));
    Forbid();
    if (!WorkbenchBase->wb_WBStarted) {
    	struct Segment *seg;
    	BPTR wbseg = 0;

    	seg = FindSegment("Workbook", NULL, TRUE);
    	if (seg != NULL)
    	    wbseg = seg->seg_Seg;
    	D(bug("StartWorkbench: DOSBase = %p, Segment = %p\n", DOSBase, BADDR(wbseg)));
    	if (wbseg) {
	    struct TagItem tags[] =
	    {
	    	/* FIXME: check tags */
		{ NP_Entry, (IPTR)BADDR(wbseg) },
		{ NP_WindowPtr, (IPTR)-1 },
		{ NP_ConsoleTask, (IPTR)BNULL },
		{ NP_CurrentDir, (IPTR)BNULL },
		{ NP_Name, (IPTR)"Workbench" },
		{ NP_Cli, TRUE },
		{ NP_Priority, 1 },
		{ TAG_END , 0 }
	    };
     	    if (CreateNewProc((struct TagItem *)tags)) {
     	    	rc = TRUE;
     	    	WorkbenchBase->wb_WBStarted = TRUE;
     	    }
    	}
    }
    Permit();
    return rc;
        
    AROS_LIBFUNC_EXIT
} /* StartWorkbench() */
