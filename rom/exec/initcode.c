/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Initialize resident modules
    Lang: english
*/
#include <aros/debug.h>
#include <exec/resident.h>
#include <proto/exec.h>

#include "exec_debug.h"
#include "exec_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH2(void, InitCode,

/*  SYNOPSIS */
	AROS_LHA(ULONG, startClass, D0),
	AROS_LHA(ULONG, version, D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 12, Exec)

/*  FUNCTION
	Traverse the ResModules array and InitResident() all modules with
	versions greater than or equal to version, and of a class equal to
	startClass.

    INPUTS
	startClass - which type of module to start
	version - a version number

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    IPTR *list = SysBase->ResModules;

    DINITCODE("enter InitCode(0x%02lx, %ld)", startClass, version);

    if (list)
    {
	while (*list)
	{
	    struct Resident *res;

            /* on amiga, if bit 31 is set then this points to another list of
             * modules rather than pointing to a single module. bit 31 is
             * inconvenient on architectures where code may be loaded above
             * 2GB. on these platforms we assume aligned pointers and use bit
             * 0 instead */
#ifdef __mc68000__
	    if (*list & 0x80000000)
	    {
	    	list = (IPTR *)(*list & 0x7fffffff); 
#else
            if (*list & 0x1)
            {
            	list = (IPTR *)(*list & ~(IPTR)0x1);
#endif
            	continue;
            }

	    res = (struct Resident *)*list++;

	    if ((res->rt_Version >= version) && (res->rt_Flags & startClass))
	    {
		DINITCODE("calling InitResident (%ld %02lx \"%s\")",
		    res->rt_Pri, res->rt_Flags, res->rt_Name);
		InitResident(res, BNULL);
	    }
	    	D(else bug("NOT calling InitResident (%d %02x \"%s\")\n",
		    res->rt_Pri, res->rt_Flags, res->rt_Name));
	}
    }

    DINITCODE("leave InitCode(0x%02lx, %ld)", startClass, version);

    AROS_LIBFUNC_EXIT
} /* InitCode */
