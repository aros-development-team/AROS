/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Search a resident module by name
    Lang: english
*/

#include <string.h>
#include <exec/resident.h>
#include <proto/exec.h>

#include "exec_debug.h"
#include "exec_intern.h"
#include "exec_util.h"

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

    IPTR *ptr;

    DFINDRESIDENT("FindResident(\"%s\")", name);    

    ptr = InternalFindResident(name, SysBase->ResModules);
    if (ptr)
    {
	DFINDRESIDENT("Found at 0x%p", *ptr);
	return (struct Resident *)*ptr;
    }

    DFINDRESIDENT("Not found");
    return NULL;

    AROS_LIBFUNC_EXIT
} /* FindResident */

IPTR *InternalFindResident(const UBYTE *name, IPTR *list)
{
    if (list)
    {
	while (*list)
	{
            /*
             * On amiga, if bit 31 is set then this points to another list of
             * modules rather than pointing to a single module. bit 31 is
             * inconvenient on architectures where code may be loaded above
             * 2GB. on these platforms we assume aligned pointers and use bit
             * 0 instead
             */
	    if (*list & RESLIST_NEXT)
	    {
	    	list = (IPTR *)(*list & ~RESLIST_NEXT);
	    	continue;
	    }

	    if (!(strcmp( ((struct Resident *)*list)->rt_Name, name)))
		return list;

	    list++;
	}
    }
    return NULL;
}
