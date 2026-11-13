/*
    Copyright (C) 2020, The AROS Development Team. All rights reserved.
*/

#define __KERNEL_NOLIBBASE__
#define __KERNEL_NOEXTERNBASE__

#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/arossupport.h>

#include <aros/x86_64/cpucontext.h>

#include LC_LIBDEFS_FILE

/* Task Info */
int HVDEBUGDumpCPUCtx(APTR ctx)
{
    struct ExceptionContext *regs = (struct ExceptionContext *)ctx;

    kprintf("CPU Context @ 0x%p:\n", ctx);
    kprintf("   -   Flags=0x%08X\n", regs->Flags);
    kprintf("   -   stack=%04x:%012x rflags=%016x ip=%04x:%012x ds=0x%04X\n",
                 regs->ss, regs->rsp, regs->rflags, regs->cs, regs->rip, regs->ds);
    kprintf("   -   rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n", regs->rax, regs->rbx, regs->rcx, regs->rdx);
    kprintf("   -   rsi=%016lx rdi=%016lx rbp=%016lx rsp=%016lx\n", regs->rsi, regs->rdi, regs->rbp, regs->rsp);
    kprintf("   -   r08=%016lx r09=%016lx r10=%016lx r11=%016lx\n", regs->r8, regs->r9, regs->r10, regs->r11);
    kprintf("   -   r12=%016lx r13=%016lx r14=%016lx r15=%016lx\n", regs->r12, regs->r13, regs->r14, regs->r15);
}

int HVDEBUGCompareCtx(APTR ctxA, APTR ctxB)
{
    struct ExceptionContext *regsA = (struct ExceptionContext *)ctxA;
    struct ExceptionContext *regsB = (struct ExceptionContext *)ctxB;
    
    if ((regsA->Flags    == regsB->Flags)       &&
        (regsA->ss       == regsB->ss)          &&
        (regsA->rsp      == regsB->rsp)         &&
        (regsA->rflags   == regsB->rflags)      &&
        (regsA->cs       == regsB->cs)          &&
        (regsA->rip      == regsB->rip)         &&
        (regsA->ds       == regsB->ds)          &&
        (regsA->rax      == regsB->rax)         &&
        (regsA->rbx      == regsB->rbx)         &&
        (regsA->rcx      == regsB->rcx)         &&
        (regsA->rdx      == regsB->rdx)         &&
        (regsA->rsi      == regsB->rsi)         &&
        (regsA->rdi      == regsB->rdi)         &&
        (regsA->rbp      == regsB->rbp)         &&
        (regsA->rsp      == regsB->rsp)         &&
        (regsA->r8       == regsB->r8)          &&
        (regsA->r9       == regsB->r9)          &&
        (regsA->r10      == regsB->r10)         &&
        (regsA->r11      == regsB->r11)         &&
        (regsA->r12      == regsB->r12)         &&
        (regsA->r13      == regsB->r13)         &&
        (regsA->r14      == regsB->r14)         &&
        (regsA->r15      == regsB->r15))
    {
        return 0;
    }
    return 2;
}