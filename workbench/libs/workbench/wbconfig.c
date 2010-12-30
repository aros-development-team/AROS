/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id: wbinfo.c 30792 2009-03-07 22:40:04Z neil $

    Undocumented WBConfig
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

    AROS_LH2(BOOL, WBConfig,
/*  SYNOPSIS */
    AROS_LHA(ULONG,            unk1,   D0),
    AROS_LHA(ULONG,            unk2,   D1),

/*  LOCATION */
    struct WorkbenchBase *, WorkbenchBase, 14, Workbench)

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
    
    bug("WBConfig() not implemented. P1=%08x P2=%08x\n", unk1, unk2);
    return 0;
        
    AROS_LIBFUNC_EXIT
} /* WBConfig() */
