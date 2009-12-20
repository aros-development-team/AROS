/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id: coldreboot.S 30792 2009-03-07 22:40:04Z neil $

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
	This function is not really necessary, and could be left unimplemented
	on many systems. It is best when using this function to allow the memory
	contents to remain as they are, since some programs may use this
	function when installing resident modules.

    HISTORY

******************************************************************************/

#include <aros/x86_64/asm.h>

	    .text
	    .globl	AROS_SLIB_ENTRY(ColdReboot,Exec)
	    _FUNCTION(AROS_SLIB_ENTRY(ColdReboot,Exec))
AROS_SLIB_ENTRY(ColdReboot,Exec):

#ifdef DEBUG
	    hlt
#else
	    movl	$-1,%eax
	    int		$0x80
#endif
