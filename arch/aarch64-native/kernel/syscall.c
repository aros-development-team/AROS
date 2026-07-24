/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    AArch64 SVC (syscall) handler.
*/

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <aros/aarch64/cpucontext.h>

#include <stddef.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_cpu.h"
#include "kernel_intern.h"
#include "kernel_scheduler.h"
#include "kernel_intr.h"
#include "kernel_syscall.h"
#include "io.h"

extern char __text_start;
extern char __text_end;

#ifndef _CUSTOM
#define _CUSTOM NULL
#endif

#define DREGS(x)
#define D(x)

/*
 * Clean and invalidate the entire data/unified cache hierarchy by set/way,
 * up to the Level of Coherency. Used for the whole-cache request, where we
 * have no address range to walk by VA.
 */
static void dcache_clean_invalidate_all(void)
{
    uint64_t clidr;
    uint32_t loc, level;

    asm volatile("mrs %0, clidr_el1" : "=r"(clidr));
    loc = (clidr >> 24) & 0x7;

    for (level = 0; level < loc; level++)
    {
        uint32_t ctype = (clidr >> (level * 3)) & 0x7;
        uint64_t ccsidr;
        uint32_t linesize, ways, sets, way_shift;
        int32_t way, set;

        if (ctype < 2)
            continue;   /* No data or unified cache at this level */

        /* Select this cache level and read its geometry */
        asm volatile("msr csselr_el1, %0" :: "r"((uint64_t)(level << 1)));
        asm volatile("isb");
        asm volatile("mrs %0, ccsidr_el1" : "=r"(ccsidr));

        linesize  = (ccsidr & 0x7) + 4;         /* log2(line size in bytes) */
        ways      = (ccsidr >> 3) & 0x3ff;      /* associativity - 1 */
        sets      = (ccsidr >> 13) & 0x7fff;    /* number of sets - 1 */
        way_shift = __builtin_clz(ways);        /* position of the way field */

        for (way = ways; way >= 0; way--)
        {
            for (set = sets; set >= 0; set--)
            {
                uint64_t sw = ((uint64_t)level << 1)
                            | ((uint64_t)way << way_shift)
                            | ((uint64_t)set << linesize);
                asm volatile("dc cisw, %0" :: "r"(sw));
            }
        }
    }

    asm volatile("dsb sy" ::: "memory");
    asm volatile("isb" ::: "memory");
}

void cache_clear_e(void *addr, uint64_t length, uint64_t flags)
{
    void *start, *end, *p;

    /*
     * Whole-cache request (CacheClearU passes addr==NULL, length==~0).
     * We must not walk the entire VA range by VA: unmapped 2MB blocks
     * would raise a translation fault on real hardware (QEMU treats DC/IC
     * ops as NOPs and hides it). Use set/way for the data cache and IALLU
     * for the instruction cache instead.
     */
    if (addr == NULL && length == 0xffffffff)
    {
        if (flags & (CACRF_ClearD | CACRF_InvalidateD))
            dcache_clean_invalidate_all();
        if (flags & CACRF_ClearI)
        {
            asm volatile("ic iallu" ::: "memory");
            asm volatile("dsb sy" ::: "memory");
            asm volatile("isb" ::: "memory");
        }
        return;
    }

    start = (void *)((uintptr_t)addr & ~63);
    end   = (void *)(((uintptr_t)addr + length + 63) & ~63);

    D(bug("[Kernel] CacheClearE from %p to %p, flags %lx\n", start, end, flags));

    /*
     * Clean (and/or invalidate) the data cache first, then a DSB so those
     * writes reach the point of unification before we invalidate the
     * instruction cache. Without the intervening DSB the A53 may refill an
     * invalidated I-line from not-yet-cleaned data.
     */
    if (flags & CACRF_ClearD)
    {
        for (p = start; p < end; p += 64)
            asm volatile("dc civac, %0" :: "r"(p));
    }
    else if (flags & CACRF_InvalidateD)
    {
        for (p = start; p < end; p += 64)
            asm volatile("dc ivac, %0" :: "r"(p));
    }
    asm volatile("dsb ish" ::: "memory");

    if (flags & CACRF_ClearI)
    {
        for (p = start; p < end; p += 64)
            asm volatile("ic ivau, %0" :: "r"(p));
        asm volatile("dsb ish" ::: "memory");
        asm volatile("isb" ::: "memory");
    }
}

void handle_syscall(void *regs)
{
    uint64_t esr;
    uint32_t svc_no;

    /*
     * In AArch64, the SVC immediate is encoded in ESR_EL1.ISS[15:0].
     * No need to read the instruction from memory.
     */
    asm volatile("mrs %0, esr_el1" : "=r"(esr));
    svc_no = esr & 0xffff;

    D(bug("[Kernel] ## SVC %d @ 0x%p\n", svc_no, ((struct ExceptionContext *)regs)->pc));

    if (svc_no <= SC_USERSTATE || svc_no == SC_REBOOT)
    {
        DREGS(cpu_DumpRegs(regs));

        switch (svc_no)
        {
            case SC_CLI:
            {
                D(bug("[Kernel] ## CLI...\n"));
                /* Set IRQ mask bit in saved SPSR_EL1 */
                ((struct ExceptionContext *)regs)->cpsr |= PSTATE_I_BIT;
                break;
            }

            case SC_STI:
            {
                D(bug("[Kernel] ## STI...\n"));
                /* Clear IRQ mask bit in saved SPSR_EL1 */
                ((struct ExceptionContext *)regs)->cpsr &= ~PSTATE_I_BIT;
                break;
            }

            case SC_SUPERSTATE:
            {
                D(bug("[Kernel] ## SUPERSTATE... (0x%lx ->", ((struct ExceptionContext *)regs)->cpsr));
                /* Change return EL to EL1h and mask IRQs */
                ((struct ExceptionContext *)regs)->cpsr &= ~(CPUMODE_MASK);
                ((struct ExceptionContext *)regs)->cpsr |= (PSTATE_I_BIT | CPUMODE_SYSTEM);
                D(bug(" 0x%lx)\n", ((struct ExceptionContext *)regs)->cpsr));
                break;
            }

            case SC_GETCPUNUMBER:
            {
                ((struct ExceptionContext *)regs)->x[0] = GetCPUNumber();
                break;
            }

            case SC_USERSTATE:
            {
                D(bug("[Kernel] ## USERSTATE... (0x%lx ->", ((struct ExceptionContext *)regs)->cpsr));
                /*
                 * Counterpart of SC_SUPERSTATE: return the caller to EL1t
                 * (task mode) and re-enable IRQs. The caller (exec's
                 * UserState()) restores SP_EL0 itself after the SVC.
                 */
                ((struct ExceptionContext *)regs)->cpsr &= ~(CPUMODE_MASK | PSTATE_I_BIT);
                ((struct ExceptionContext *)regs)->cpsr |= CPUMODE_USER;
                D(bug(" 0x%lx)\n", ((struct ExceptionContext *)regs)->cpsr));
                break;
            }

            case SC_ISSUPERSTATE:
            {
                D(bug("[Kernel] ## ISSUPERSTATE... "));
                /* Check if saved SPSR indicates EL0 */
                ((struct ExceptionContext *)regs)->x[0] = ((((struct ExceptionContext *)regs)->cpsr & CPUMODE_MASK) != CPUMODE_USER);
                D(bug("%ld\n", ((struct ExceptionContext *)regs)->x[0]));
                break;
            }

            case SC_REBOOT:
            {
                D(bug("[Kernel] ## REBOOT...\n"));
                /*
                 * Trigger a full chip reset through the BCM283x PM
                 * watchdog. Jumping to address 0 (the arm32 idiom) does
                 * not work here: 0 is ordinary boot memory on AArch64,
                 * not a reset vector.
                 */
                uintptr_t pm = (uintptr_t)__arm_arosintern.ARMI_PeripheralBase + 0x100000;
                uint32_t rstc = rd32le(pm + 0x1c);          /* PM_RSTC */
                wr32le(pm + 0x24, 0x5a000000 | 10);         /* PM_WDOG: short timeout */
                wr32le(pm + 0x1c, 0x5a000000 | (rstc & ~0x30) | 0x20); /* full reset */
                for (;;)
                    asm volatile("wfe");
                break;
            }

            case SC_CACHECLEARE:
            {
                D(bug("[Kernel] ## CACHECLEARE...\n"));
                void *address = (void *)((struct ExceptionContext *)regs)->x[0];
                uint64_t length = ((struct ExceptionContext *)regs)->x[1];
                uint64_t flags = ((struct ExceptionContext *)regs)->x[2];

                cache_clear_e(address, length, flags);
                break;
            }

            default:
            {
                /*
                 * Execute core_SysCall only when returning to user mode.
                 * This prevents Cause() from being called before an IRQ
                 * is fully handled.
                 */
                uint64_t mode = ((struct ExceptionContext *)regs)->cpsr & CPUMODE_MASK;
                if (mode == CPUMODE_USER)
                    core_SysCall(svc_no, regs);
                break;
            }
        }
    }
    else
    {
        D(bug("[Kernel] ## SVC : ILLEGAL SVC %d!\n", svc_no));
        return;
    }

    D(bug("[Kernel] ## SVC returning ..\n"));
}
