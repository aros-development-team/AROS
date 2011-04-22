/* CPU-specific definitions for x86-64 */

#ifndef KERNEL_CPU_H_
#define KERNEL_CPU_H_

#include <aros/x86_64/cpucontext.h>

#include "segments.h"

/*
 * We handle all 255 exception vectors.
 * Vectors starting from 0x20 are hardware IRQs. We distinguish them by node type.
 */
#define EXCEPTIONS_COUNT 255

/* We use native context format, no conversion needed */
#define regs_t struct ExceptionContext
/* There are no private add-ons */
#define AROSCPUContext ExceptionContext

/* We have no VBlank emulation */
#define NO_VBLANK_EMU

/* User/supervisor mode switching */
#define cpumode_t __unused char

#define goSuper() 0
#define goUser()
#define goBack(mode)

/* A command to issue a syscall */
#define krnSysCall(num) asm volatile("int $0x80"::"a"(num):"memory")

#define IN_USER_MODE						\
({								\
    short __value;						\
    __asm__ __volatile__ ("mov %%cs,%%ax":"=a"(__value));	\
    (__value & 0x03);						\
})

#define PRINT_CPUCONTEXT(regs)											\
    bug("[Kernel] Flags=0x%08X\n", regs->Flags);								\
    bug("[Kernel] stack=%04x:%012x rflags=%016x ip=%04x:%012x ds=0x%04X\n",					\
                 regs->ss, regs->rsp, regs->rflags, regs->cs, regs->rip, regs->ds);				\
    bug("[Kernel] rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n", regs->rax, regs->rbx, regs->rcx, regs->rdx);	\
    bug("[Kernel] rsi=%016lx rdi=%016lx rbp=%016lx rsp=%016lx\n", regs->rsi, regs->rdi, regs->rbp, regs->rsp);	\
    bug("[Kernel] r08=%016lx r09=%016lx r10=%016lx r11=%016lx\n", regs->r8, regs->r9, regs->r10, regs->r11);	\
    bug("[Kernel] r12=%016lx r13=%016lx r14=%016lx r15=%016lx\n", regs->r12, regs->r13, regs->r14, regs->r15);

#endif
