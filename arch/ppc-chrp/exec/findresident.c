/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Search a resident module by name
    Lang: english
*/
#include "exec_intern.h"
#include <string.h>
#include <exec/resident.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

        AROS_LH1(struct Resident *, FindResident,

/*  SYNOPSIS */
        AROS_LHA(const UBYTE *, name, A1),

/*  LOCATION */
        struct ExecBase *, SysBase, 16, Exec)

/*  FUNCTION
        Search for a Resident module in the system resident list.

    INPUTS
        name - pointer to the name of a Resident module to find

    RESULT
        pointer to the Resident module (struct Resident *), or null if
        not found.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    IPTR *list;

    list = SysBase->ResModules;

    if(list)
    {
        while(*list)
        {
            /*
                If bit 31 is set, this doesn't point to a Resident module, but
                to another list of modules.
            */
            if(*list & 1) list = (IPTR *)(*list & 0xfffffffffffffffe);

            if(!(strcmp( ((struct Resident *)*list)->rt_Name, name)) )
            {
                return (struct Resident *)*list;
            }

            list++;
        }
    }

    return NULL;
    AROS_LIBFUNC_EXIT
} /* FindResident */
