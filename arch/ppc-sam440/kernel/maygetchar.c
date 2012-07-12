#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <asm/amcc440.h>
#include <asm/io.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0(int, KrnMayGetChar,

/*  SYNOPSIS */

/*  LOCATION */
	struct KernelBase *, KernelBase, 26, Kernel)

/*  FUNCTION
	Read a single character from low-level debug input stream

    INPUTS
	None

    RESULT
	An ASCII code of the character or -1 if there's no character
	available

    NOTES
	This function never waits. If there is no character available on
	the stream it just returns with -1

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (inb(UART0_LSR) & UART_LSR_DR)
        return inb(UART0_RBR);

    return -1;

    AROS_LIBFUNC_EXIT
}
