/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Emit one character via raw IO
    Lang: english
*/

#include <aros/config.h>

#ifdef DO_SERIAL_DEBUG
# include <asm/registers.h>
#endif

void vputc(unsigned char chr);

#include <proto/exec.h>

/* See rom/exec/rawputchar.c for documentation */

AROS_LH1(void, RawPutChar,
    AROS_LHA(UBYTE, chr, D0),
    struct ExecBase *, SysBase, 86, Exec)
{
	AROS_LIBFUNC_INIT
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
