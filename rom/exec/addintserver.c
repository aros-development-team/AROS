/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add interrupt client to chain of interrupt server
    Lang: english
*/
#include <aros/config.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
#include <hardware/custom.h>
#include <hardware/intbits.h>
#endif

#include <proto/exec.h>
#include <aros/libcall.h>

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

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
    struct Custom *custom = (struct Custom *)(void **)0xdff000;
#endif

    Disable();

    Enqueue((struct List *)SysBase->IntVects[intNumber].iv_Data, (struct Node *)interrupt);

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
    /*
	Enable the chipset interrupt if run on a native Amiga.
    */
    custom->intena = (UWORD)(INTF_SETCLR|(1L<<intNumber));
#endif

    Enable();

    AROS_LIBFUNC_EXIT
} /* AddIntServer */
