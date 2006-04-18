/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: traps.c 16672 2003-03-08 19:33:25Z bergers $
*/

#include <asm/linkage.h>
#include <exec/ptrace.h>
#include <exec/alerts.h>
#include <proto/exec.h>
#include <aros/debug.h>
#include <asm/cpu.h>
#include <asm/registers.h>

#include "traps.h"
#include "pp_exec_internal.h"

void do_TRAP(struct pt_regs regs)
{
	AROS_GET_SYSBASE
	ULONG alert = 0;
#if 0
	kprintf("*** trap: eip = %x eflags = %x  ds = %x sp ~= %x\n",
	        regs.eip, regs.eflags, regs.xds, &regs);

	switch (regs.orig_eax) {
		case 0:
			alert = ACPU_DivZero;
			break;
		case 6:
			alert = ACPU_InstErr;
			break;
		default:
			alert = AT_DeadEnd | 0x100 | regs.orig_eax;
	}
#endif
	Alert(alert);
}

void Init_Traps(void) {
	INSTALL_TRAP_HANDLER(TRAP_1, _sys_trap1_handler);
}
