/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef ASM_PTRACE_H
#define ASM_PTRACE_H

/* this struct defines the way the registers are stored on the 
   stack during a system call. */

struct pt_regs {
	int  xes;
	int  xds;
	long edx;
	long ecx;
	long eax;
	long orig_eax;
	long eip;
	int  xcs;
	long eflags;
	long esp;
	int  xss;
};

#define user_mode(regs) ((3 & (regs)->xcs))
#define instruction_pointer(regs) ((regs)->eip)

#endif
