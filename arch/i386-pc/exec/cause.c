/*
    Copyright (C) 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Cause() - Cause a software interrupt.
    Lang: english
*/

/* Change this to <exec_intern.h> if you move this file... */
#include "exec_intern.h"

/*****************************************************************************

    NAME */
#include <exec/interrupts.h>
#include <proto/exec.h>

	AROS_LH1(void, Cause,

/*  SYNOPSIS */
	AROS_LHA(struct Interrupt *, softint, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 30, Exec)

/*  FUNCTION
	Schedule a software interrupt to occur. If the processor is
	currently running a user task, then the software interrupt will
	prempt the current task and run itself. From a real interrupt, it
	will queue the software interrupt for a later time.

	Software interrupts are useful from hardware interrupts if you
	wish to defer your processing down to a lower level. They can
	also be used in some special cases of device I/O. The timer.device
	and audio.device allow software interrupt driven timing and
	audio output respectively.

	Software interrupts are restricted to 5 different priority levels,
	+32, +16, 0, -16, -32.

	Software interrupts can only be scheduled once.

	The software interrupt is called with the following prototype:

	AROS_UFH3(void, YourIntCode,
	    AROS_UFHA(APTR, interruptData, A1),
	    AROS_UFHA(APTR, interruptCode, A5),
	    AROS_UFHA(struct ExecBase *, SysBase, A6))

	The interruptData is the value of the is_Data field, interruptCode
	is the value of the is_Code field - it is included for historical
	and compatibility reasons. You can ignore the value of interruptCode,
	but you must declare it.

    INPUTS
	softint     -   The interrupt you wish to schedule. When setting up
			you should set the type of the interrupt to either
			NT_INTERRUPT or NT_UNKNOWN.

    RESULT
	The software interrupt will be delivered, or queued for later
	delivery.

    NOTES
	No bounds checking on the software interrupt priority is done.
	Passing a bad priority to the system can have a strange effect.

    EXAMPLE

    BUGS
	Older versions of the Amiga operating system require that the
	software interrupts preserve the A6 register.

	Software interrupts which are added from a software interrupt of
	lower priority may not be called immediately.

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    aros_print_not_implemented("Cause");

    AROS_LIBFUNC_EXIT
} /* Cause() */
