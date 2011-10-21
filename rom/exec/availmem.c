/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Tell how much memory is available.
    Lang: english
*/

#include <exec/alerts.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/macros.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "memory.h"
#include "mungwall.h"

/*****************************************************************************

    NAME */

	AROS_LH1(ULONG, AvailMem,

/*  SYNOPSIS */
	AROS_LHA(ULONG, attributes, D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 36, Exec)

/*  FUNCTION
	Return either the total available memory or the largest available
	chunk of a given type of memory.

    INPUTS
	attributes - The same attributes you would give to AllocMem().

    RESULT
	Either the total of the available memory or the largest chunk if
	MEMF_LARGEST is set in the attributes.

    NOTES
	Due to the nature of multitasking the returned value may already
	be obsolete when this function returns.

    EXAMPLE
	Print the total available memory.

	printf("Free memory: %lu bytes\n", AvailMem(0));

	Print the size of the largest chunk of chip memory.

	printf("Largest chipmem chunk: %lu bytes\n",
	       AvailMem(MEMF_CHIP|MEMF_LARGEST));

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG ret;
    struct TraceLocation tp = CURRENT_LOCATION("AvailMem");

    ret = nommu_AvailMem(attributes, SysBase);

    if (attributes & MEMF_CLEAR)
	MungWall_Scan(NULL, &tp, SysBase);

    return ret;

    AROS_LIBFUNC_EXIT
} /* AvailMem */

