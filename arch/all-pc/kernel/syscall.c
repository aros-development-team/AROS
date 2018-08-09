/*
    Copyright © 2017-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <asm/cpu.h>
#include <asm/io.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_globals.h"
#include "kernel_interrupts.h"
#include "kernel_intern.h"
#include "kernel_intr.h"
#include "kernel_scheduler.h"
#include "kernel_syscall.h"
#include "cpu_traps.h"

#define D(x)
#define DSYSCALL(x)

#if (__WORDSIZE != 64)
extern void core_Kick(struct TagItem *msg, void *target);
extern void kernel_cstart(const struct TagItem *msg);
#endif

int core_SysCallHandler(struct ExceptionContext *regs, struct KernelBase *KernelBase, void *HandlerData2)
{
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct syscallx86_Handler *scHandler;
    ULONG sc =
#if (__WORDSIZE == 64)
        regs->rax;
#else
        regs->eax;
#endif
    BOOL systemSysCall = TRUE;

    /* Syscall number is actually ULONG (we use only eax) */
    DSYSCALL(bug("[Kernel] %s: Syscall %08x\n", __func__, sc));

    ForeachNode(&pdata->kb_SysCallHandlers, scHandler)
    {
        if ((ULONG)((IPTR)scHandler->sc_Node.ln_Name) == sc)
        {
            DSYSCALL(bug("[Kernel] %s: calling handler @ 0x%p\n", __func__, scHandler));
            scHandler->sc_SysCall(regs);
            if (scHandler->sc_Node.ln_Type == 1)
                systemSysCall = FALSE;
        }
    }

    if (systemSysCall && INTR_FROMUSERMODE)
    {
        DSYSCALL(bug("[Kernel] %s: User-mode syscall\n", __func__));

        /* Disable interrupts for a while */
        __asm__ __volatile__("cli; cld;");

        core_SysCall(sc, regs);
    }

    DSYSCALL(bug("[Kernel] %s: Returning from syscall...\n", __func__));

    return TRUE;
}

BOOL krnAddSysCallHandler(struct PlatformData *pdata, struct syscallx86_Handler *newscHandler, BOOL custom, BOOL force)
{
    struct syscallx86_Handler *scHandler;

    D(bug("[Kernel] %s(%08x)\n", __func__, newscHandler->sc_Node.ln_Name));

    /* Unless the 'force' flag is set, bale out if there is already a handler
     * of the same type (note that we're not comparing strings here - ln_Name
     * is abused) */
    if (!force)
    {
        ForeachNode(&pdata->kb_SysCallHandlers, scHandler)
        {
            if (scHandler->sc_Node.ln_Name == newscHandler->sc_Node.ln_Name)
                return FALSE;
        }
    }

    if (custom)
        newscHandler->sc_Node.ln_Type = 1;
    else
        newscHandler->sc_Node.ln_Type = 0;

    D(bug("[Kernel] %s: Registering handler...\n", __func__));
    AddTail(&pdata->kb_SysCallHandlers, &newscHandler->sc_Node);

    return TRUE;
}

/* Default x86 syscall handlers */

void X86_HandleChangePMStateSC(struct ExceptionContext *regs)
{
    UBYTE pmState =
#if (__WORDSIZE==64)
        (UBYTE)regs->rbx;
#else
        (UBYTE)regs->ebx;
#endif

    D(bug("[Kernel] %s(0x%02x)\n", __func__, pmState));
    
    if (pmState == 0xFF)
    {
        D(bug("[Kernel] %s: STATE 0xFF - Attempting Cold Reboot...\n", __func__));

        /*
         * On some machines (new PCs without a PS/2 controller), this might
         * not work.
         */

        outb(0xFE, 0x64);
    }
    else if (pmState == 0)
    {
        D(bug("[Kernel] %s: STATE 0x00 - Halting...\n", __func__));

        /*
                there is no way to power off by default,
                so just halt the cpu.. */

        while (1) asm volatile("hlt");
    }
    else if (pmState == 0x90)
    {
        /* Sleep almost forever ;) */
        __asm__ __volatile__("sti; hlt; cli");

        D(bug("[Kernel] %s: Woke from sleep; checking for softints...\n",
            __func__);)
        if (SysBase->SysFlags & SFF_SoftInt)
            core_Cause(INTB_SOFTINT, 1L << INTB_SOFTINT);
    }
    else
    {
        /* We can't handle any other states atm =/ */
        D(bug("[Kernel] %s: UNHANDLED STATE 0x%02x\n", __func__, pmState));
    }
}

struct syscallx86_Handler x86_SCChangePMStateHandler =
{
    {
        .ln_Name = (APTR)SC_X86CHANGEPMSTATE
    },
    (APTR)X86_HandleChangePMStateSC
};

/* Generic warm-reset handler */

void X86_HandleRebootSC()
{
    D(bug("[Kernel] %s: Warm restart, stack 0x%p\n", __func__, AROS_GET_SP));

    /*
     * Restart the kernel with a double stack swap. This doesn't return.
     * Double swap guarantees that core_Kick() is called when SP is set to a
     * dynamically allocated emergency stack and not to boot stack.
     * Such situation is rare but can occur in the following situation:
     * 1. Boot task calls SuperState(). Privilege changed, but stack is manually reset
     *    back into our .bss space.
     * 2. Boot task crashes. Privilege doesn't change this time, ESP/RSP is not changed.
     * 3. If we call core_Kick() right now, we are dead (core_Kick() clears .bss).
     */
#if (__WORDSIZE == 64)
    __asm__ __volatile__(
        "cli\n\t"
        "cld\n\t"
        "movq %0, %%rsp\n\t"
        "jmp *%1\n"
        ::"r"(__KernBootPrivate->SystemStack + STACK_SIZE), "r"(core_Kick), "D"(BootMsg), "S"(kernel_cstart));
#else
    __asm__ __volatile__(
        "cli\n\t"
        "cld\n\t"
        "movl %0, %%esp\n\t"
        "pushl %3\n\t"
        "pushl %2\n\t"
        "call *%1\n"
        ::"r"(0x1000), "r"(core_Kick), "r"(BootMsg), "r"(kernel_cstart));
#endif
}

struct syscallx86_Handler x86_SCRebootHandler =
{
    {
        .ln_Name = (APTR)SC_REBOOT
    },
    (APTR)X86_HandleRebootSC
};

struct syscallx86_Handler x86_SCSupervisorHandler =
{
    {
        .ln_Name = (APTR)SC_SUPERVISOR
    },
    (APTR)core_Supervisor
};
