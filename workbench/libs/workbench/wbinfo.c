/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

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
        lock - Lock to directory or disk
        name - Name of the object in directory.
               Note: also for def icons, name has to be passed without .info
               extension to be able to edit def icon attributes. Passing a
               name.info should open information on the .info file itself (a
               binary file without any icon attributes). This behavior is
               confirmed with Workbench.

    RESULT

    NOTES

    EXAMPLE

    BUGS
        screen argument is currently ingored

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
