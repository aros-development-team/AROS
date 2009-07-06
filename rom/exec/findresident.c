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
            /* on amiga, if bit 31 is set then this points to another list of
             * modules rather than pointing to a single module. bit 31 is
             * inconvenient on architectures where code may be loaded above
             * 2GB. on these platforms we assume aligned pointers and use bit
             * 0 instead */
#ifdef __mc680000__
	    if(*list & 0x80000000) list = (IPTR *)(*list & 0x7fffffff);
#else
            if(*list & 0x1) list = (IPTR *)(*list & ~(IPTR)0x1);
#endif

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
