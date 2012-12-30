/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Free memory allocated by AllocPooled().
    Lang: english
*/

#include <aros/libcall.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "memory.h"

/*****************************************************************************

    NAME */

	AROS_LH3(void,FreePooled,

/*  SYNOPSIS */
	AROS_LHA(APTR, poolHeader,A0),
	AROS_LHA(APTR, memory,    A1),
	AROS_LHA(IPTR, memSize,   D0),

/* LOCATION */
	struct ExecBase *, SysBase, 119, Exec)

/*  FUNCTION
	Free memory allocated out of a private memory pool.

    INPUTS
	poolHeader - Handle of the memory pool
	memory	   - Pointer to the memory
	memSize    - Size of the memory chunk

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CreatePool(), DeletePool(), AllocPooled()

    INTERNALS
	In AROS memory allocated from pool remembers where it came from.
	Because of this poolHeader is effectively ignored and is present
	only for compatibility reasons. However, do not rely on this! For
	other operating systems of Amiga(tm) family this is not true!

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TraceLocation tp = CURRENT_LOCATION("FreePooled");

    InternalFreePooled(memory, memSize, &tp, SysBase);

    AROS_LIBFUNC_EXIT
} /* FreePooled */
