/*
    Copyright � 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>

#include <hardware/mx51_uart.h>

#include <kernel_base.h>
#include <kernel_debug.h>

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

    volatile MX51_UART * const uart = (MX51_UART *)UART1_BASE_ADDR;

    if (uart->USR2 & UART_USR2_RDR)
        return uart->URXD;
    else
        return -1;

    AROS_LIBFUNC_EXIT
}
