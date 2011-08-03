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
    	This is actually internal function. There's no sense to call it from
    	within user software.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    IPTR *list;

    DINITCODE("enter InitCode(0x%02lx, %ld)", startClass, version);

    if (startClass == RTF_COLDSTART)
    {
    	/*
    	 * When the system enters RTF_COLDSTART level, it's a nice time to pick up
    	 * KickTags.
    	 * We could call this from within exec init code, but InitKickTags() function
    	 * will replace SysBase->ResModules if it finds some KickTags. In order to
    	 * simplify things down, we keep it here, before we start using the list.
    	 */
	InitKickTags(SysBase);
    }

    list = SysBase->ResModules;
    if (list)
    {
	while (*list)
	{
	    struct Resident *res;

            /*
             * On Amiga(tm), if bit 31 is set then this points to another list of
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
