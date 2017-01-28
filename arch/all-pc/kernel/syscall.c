/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
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

#if (__WORDSIZE != 64)
extern void core_Kick(struct TagItem *msg, void *target);
extern void kernel_cstart(const struct TagItem *msg);
#endif

BOOL krnAddSysCallHandler(struct PlatformData *pdata, struct syscallx86_Handler *newscHandler, BOOL force)
{
    struct syscallx86_Handler *scHandler;

    D(bug("[Kernel] %s(%08x)\n", __func__, newscHandler->sc_Node.ln_Name));

    ForeachNode(&pdata->kb_SysCallHandlers, scHandler)
    {
        if ((!force) &&
            ((ULONG)((IPTR)scHandler->sc_Node.ln_Name) == (ULONG)((IPTR)newscHandler->sc_Node.ln_Name)))
            return FALSE;
    }

    D(bug("[Kernel] %s: Registering handler...\n", __func__));
    AddTail(&pdata->kb_SysCallHandlers, &newscHandler->sc_Node);

    return TRUE;
}

/* Deafult x86 Syscall Handlers */

void X86_HandleShutdownSC()
{
    D(bug("[Kernel] %s()\n", __func__));
    /*
            there is no way to power off by default,
            so just halt the cpu.. */
    while (1) asm volatile("hlt");
}

struct syscallx86_Handler x86_SCShutdownHandler =
{
    {
        .ln_Name = (APTR)SC_X86SHUTDOWN
    },
    (APTR)X86_HandleShutdownSC
};

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
     * 2. Boot task crashes. Privilege doesn't change this time, RSP is not changed.
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
        "call *%1\n"
        ::"r"(0x1000), "r"(core_Kick), "D"(BootMsg), "S"(kernel_cstart));
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
