/*
    Copyright � 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Open the file information window for a specified file.
*/

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
    AROS_LIBBASE_EXT_DECL(struct WorkbenchBase *, WorkbenchBase)
    
    // FIXME: screen argument is ignored
    
    return OpenWorkbenchObject
    (
        "WANDERER:Tools/Info",
        WBOPENA_ArgLock, (IPTR) lock,
        WBOPENA_ArgName, (IPTR) name,
        TAG_DONE
    );
        
    AROS_LIBFUNC_EXIT
} /* WBInfo() */
