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
    AROS_LHA(BPTR,      parentlock,   A1),
    AROS_LHA(LONG,          action,   D0),

/*  LOCATION */
    struct WorkbenchBase *, WorkbenchBase, 5, Workbench)

/*  FUNCTION
 
    This function does the "magic" of letting Workbench know that
    an object has been added, changed, or removed. The name is
    the name of the object, the lock is a lock on the directory that
    contains the object. The action determines what has happened.
    If UPDATEWB_ObjectAdded, the object is either NEW or has CHANGED.
    If UPDATEWB_ObjectRemoved, the object has been deleted.

    INPUTS
    
    name         - Name of the object (without the .info)
    parentlock   - Lock on the object's parent directory.
    action       - UPDATEWB_ObjectAdded for a new or changed object
                   UPDATEWB_ObjectRemoved for a deleted object

    RESULT

    Workbench will update its display, if needed. An object that has
    been deleted will be removed from the display. An object that is
    new will be added to the respective display if it is not already
    there; if it is already there, its appearance will be changed if
    necessary.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    D(bug("UpdateWorkbench(\"%s\", LOCK %p, 0x%08x)\n", name, BADDR(parentlock), action));
    return TRUE;
        
    AROS_LIBFUNC_EXIT
} /* UpdateWorkbench() */
