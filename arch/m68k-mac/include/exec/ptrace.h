/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: ptrace.h 15191 2002-08-13 00:57:15Z bergers $
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
	long            usp;
	long            d0;
	long            d1;
	long            d2;
	long            d3;
	long            d4;
	long            d5;
	long            d6;
	long            d7;
	long            a0;
	long            a1;
	long            a2;
	long            a3;
	long            a4;
	long            a5;
	long            a6;
	unsigned short  sr;
	long            pc;
} __attribute__((packed));

#define user_mode(regs) (0 == ((1 << 13) & (regs)->sr))

#endif
