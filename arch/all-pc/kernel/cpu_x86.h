#ifndef KERNEL_CPU_X86_H
#define KERNEL_CPU_X86_H
/*
    Copyright © 2023, The AROS Development Team. All rights reserved.
    $Id$

    Desc: base x86 definitions
    Lang: english
*/

#include <aros/cpu.h>
#include <asm/cpu.h>

#if (__WORDSIZE==64)
typedef struct int_gate_64bit x86vectgate_t;
#define CPUEXCTX_REGA               regs->rax
#define CPUEXCTX_REGB               regs->rbx
#else
typedef struct int_gate_32bit x86vectgate_t;
#define CPUEXCTX_REGA               regs->eax
#define CPUEXCTX_REGB               regs->ebx
#endif

#endif /* KERNEL_CPU_X86_H */
