/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ColdReboot(), x86_64-pc implementation
    Lang: english
*/

#include <aros/x86_64/asm.h>

	    .text
	    .globl AROS_SLIB_ENTRY(ColdReboot,Exec,121)
	    _FUNCTION(AROS_SLIB_ENTRY(ColdReboot,Exec,121))
AROS_SLIB_ENTRY(ColdReboot,Exec,121):

	    call	Exec_DoResetCallbacks	// SysBase is already in rdi
	    movl	$0x100, %eax		// SC_REBOOT
	    int		$0x80
	    ret

	    .size	AROS_SLIB_ENTRY(ColdReboot,Exec,121), .-AROS_SLIB_ENTRY(ColdReboot,Exec,121)
