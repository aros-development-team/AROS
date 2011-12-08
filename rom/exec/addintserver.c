/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add interrupt client to chain of interrupt servers
    Lang: english
*/

#include <aros/debug.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include "chipset.h"

/*****************************************************************************

    NAME */

	AROS_LH2(void, AddIntServer,

/*  SYNOPSIS */
	AROS_LHA(ULONG,              intNumber, D0),
	AROS_LHA(struct Interrupt *, interrupt, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 28, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES
	This function also enables the corresponding chipset interrupt if
	run on a native Amiga.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    Disable();

    Enqueue((struct List *)SysBase->IntVects[intNumber].iv_Data, &interrupt->is_Node);
    CUSTOM_ENABLE(intNumber);

    Enable();

    AROS_LIBFUNC_EXIT
} /* AddIntServer */
