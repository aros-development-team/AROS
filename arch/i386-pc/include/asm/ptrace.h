/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef ASM_PTRACE_H
#define ASM_PTRACE_H

#include <exec/types.h>

/* this struct defines the way the registers are stored on the 
   stack during a system call. */

struct pt_regs {
    UWORD xes;
    UWORD pad1;
    UWORD xds;
    UWORD pad2;
    ULONG edx;
    ULONG ecx;
    ULONG eax;
    ULONG orig_eax;
    ULONG eip;
    UWORD xcs;
    UWORD pad3;
    ULONG eflags;
    ULONG esp;
    UWORD xss;
};

#define user_mode(regs) ((3 & (regs)->xcs))
#define instruction_pointer(regs) ((regs)->eip)

#endif
