/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Emit one character via raw IO
    Lang: english
*/

#include <aros/config.h>

#ifdef DO_SERIAL_DEBUG
# include <asm/registers.h>
#endif

void vputc(unsigned char chr);

/****************************************************************************

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
	AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)
#if 0
#ifdef DO_SERIAL_DEBUG
	/*
	 * This does not work with xcopilot...
	 */
	if (chr) {
		/*
		 * Wait until FIFO is half empty
		 */
		while (0 == (RREG_W(UTX1) & FIFO_HALF_F)) {
		}
		WREG_W(UTX1) = (UWORD)chr;
	}
#endif
#endif
	/* Don't write 0 bytes */
	if (chr) {
		vputc(chr);
	}

	AROS_LIBFUNC_EXIT
} /* RawPutChar */
