/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.

    Desc: CPU-specific definitions.
*/

#ifndef KERNEL_CPU_H_
#define KERNEL_CPU_H_

#include <aros/i386/cpucontext.h>

#include "segments.h"

/* We use native context format, no conversion needed */
#define regs_t struct ExceptionContext
/* There are no private add-ons */
#define AROSCPUContext ExceptionContext
#define CPUSSEContxtSize (sizeof(struct AROSCPUContext) + sizeof(struct FPXContext) + 15)

/* We have no VBlank emulation */
#define NO_VBLANK_EMU

/* User/supervisor mode switching - not used */
#define cpumode_t __unused char

#define goSuper() 0
#define goUser()
#define goBack(mode)

/* A command to issue a syscall */
#define krnSysCall(num) asm volatile("int $0xfe"::"a"(num):"memory")

#define IN_USER_MODE \
	({  short __value; \
	__asm__ __volatile__ ("mov %%cs,%%ax":"=a"(__value)); \
	(__value & 0x03);	})

#define PRINT_CPUCONTEXT(regs)										\
{													\
    bug("[Kernel] Flags=0x%08X\n", regs->Flags);							\
    bug("[Kernel] stack=%04x:%08x eflags=%08x eip=%04x:%08x ds=0x%04X\n",				\
                 regs->ss, regs->esp, regs->eflags, regs->cs, regs->eip, regs->ds);			\
    bug("[Kernel] eax=%08x ebx=%08x ecx=%08x edx=%08x\n", regs->eax, regs->ebx, regs->ecx, regs->edx);	\
    bug("[Kernel] esi=%08x edi=%08x ebp=%08x esp=%08x\n", regs->esi, regs->edi, regs->ebp, regs->esp);	\
}

#define SP(regs) regs->esp

#endif
