/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef ASM_PTRACE_H
#define ASM_PTRACE_H

/* 
   This struct defines the way the registers are stored on the 
   stack during a system call.
   Now you might think that the order is arbitrary, but in reality it
   is not! The highest ones, sr & pc, are from the exception frame and
   should also remain like that! 
*/

struct pt_regs {
	long            r0;
	long            r1;
	long            r2;
	long            r3;
	long            r4;
	long            r5;
	long            r6;
	long            r7;
	long            r8;
	long            r9;
	long            r10;
	long            r11;
	long            r12;
	long            sp;
	long            lr;
	long		lr_svc;
	long            cpsr;
};

#define user_mode(regs) (0x10 == ((regs)->cpsr & 0x1f))

#endif
