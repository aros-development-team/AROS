/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Emit one character via raw IO
    Lang: english
*/

#include <aros/config.h>
#include <asm/registers.h>

/*****i***********************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH1(void, RawPutChar,

/*  SYNOPSIS */
	AROS_LHA(UBYTE, chr, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 86, Exec)

/*  FUNCTION
	Emits a single character.

    INPUTS
	chr - The character to emit

    RESULT
	None.

    NOTES
	This function is for very low level debugging only.

    EXAMPLE

    BUGS

    SEE ALSO
	RawIOInit(), RawPutChar(), RawMayGetChar()

    INTERNALS

    HISTORY

*****************************************************************************/
{
	AROS_LIBFUNC_INIT
#define DO_SERIAL_DEBUG
#ifdef DO_SERIAL_DEBUG
	ULONG j = 0;
	volatile ULONG utsr1;
//	Disable();
	do {
		utsr1 = RREG_L(UTSR1);
		j++;
	} while ((0 == (utsr1 & 0x04)) && (j < 1000000));
	if (chr) {
		WREG_L(UTDR) = (ULONG)chr;
		if ('\n' == chr) {
			WREG_L(UTDR) = (ULONG)'\r';
		}
	} else {
		WREG_L(UTDR) = '\r';
		WREG_L(UTDR) = '\n';
	}
//	Enable();
#endif

	AROS_LIBFUNC_EXIT
} /* RawPutChar */
