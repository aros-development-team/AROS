/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Emit one character via raw IO
    Lang: english
*/
#include <proto/kernel.h>
#include <aros/kernel.h>
#include <proto/arossupport.h>

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

  KRNWireImpl(Putchar);	  

    /* Don't write 0 bytes */
    if (chr)
    {
	  CALLHOOKPKT(krnPutcharImpl,0,(struct TagList*)chr);
    }

    AROS_LIBFUNC_EXIT
} /* RawPutChar */
