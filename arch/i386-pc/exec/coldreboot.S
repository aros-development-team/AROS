/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ColdReboot() - Reboot the computer.
    Lang: english
*/

/*****************************************************************************

    NAME

	AROS_LH0(void, ColdReboot,

    LOCATION
	struct ExecBase *, SysBase, 121, Exec)

    FUNCTION
	This function will reboot the computer.

    INPUTS
	None.

    RESULT
	This function does not return.

    NOTES
	It can be quite harmful to call this function. It may be possible that
	you will lose data from other tasks not having saved, or disk buffers
	not being flushed. Plus you could annoy the (other) users.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include "aros/i386/asm.h"

	    .text
	    .globl	AROS_SLIB_ENTRY(ColdReboot,Exec,121)
	    _FUNCTION(AROS_SLIB_ENTRY(ColdReboot,Exec,121))
AROS_SLIB_ENTRY(ColdReboot,Exec,121):

#ifdef DEBUG
	    hlt
#else
	    pushl	4(%esp)
	    call	Exec_DoResetCallbacks
	    movl	$-1,%eax
	    int		$0x80
#endif
