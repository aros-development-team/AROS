/*
    Copyright � 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <aros/arm/cpucontext.h>

#include <stddef.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_cpu.h"
#include "kernel_intern.h"
#include "kernel_scheduler.h"
#include "kernel_intr.h"
#include "kernel_syscall.h"

extern char __text_start;
extern char __text_end;

#ifndef _CUSTOM
#define _CUSTOM NULL
#endif

#define DREGS(x)
#define D(x)

/*
    __vectorhand_swi:

    this code currently assumes the caller is in user mode (and will break from any other mode)

    r0 = passed to c handler, r1/r2 = temp
*/
asm (
    ".globl __vectorhand_swi                   \n"
    ".type __vectorhand_swi,%function          \n"
    "__vectorhand_swi:                         \n"
    VECTCOMMON_START
    "           bl      handle_syscall         \n"
    VECTCOMMON_END
);

void cache_clear_e(void *addr, uint32_t length, uint32_t flags)
{
    uint32_t count = 0;

    if (addr == NULL && length == 0xffffffff)
    {
        count = 0x8000000;
    }
    else
    {
        void *end_addr = (void*)(((uintptr_t)addr + length + 31) & ~31);
        addr = (void *)((uintptr_t)addr & ~31);
        count = (uintptr_t)(end_addr - addr) >> 5;
    }

    D(bug("[Kernel] CacheClearE from %p length %d count %d, flags %x\n", addr, length, count, flags));

    while (count--)
    {
        if (flags & CACRF_ClearD)
        {
            __asm__ __volatile__("mcr p15, 0, %0, c7, c14, 1"::"r"(addr));
        }
        if (flags & CACRF_ClearI)
        {
            __asm__ __volatile__("mcr p15, 0, %0, c7, c5, 1"::"r"(addr));
        }
        if (flags & CACRF_InvalidateD)
        {
            __asm__ __volatile__("mcr p15, 0, %0, c7, c6, 1"::"r"(addr));
        }

        addr += 32;
    }

    __asm__ __volatile__("mcr p15, 0, %0, c7, c10, 4"::"r"(addr));
}

void handle_syscall(void *regs)
{
    register unsigned int addr;
    register unsigned int swi_no;

    /* We determine the SWI number by reading in "tasks"
       program counter, subtract the instruction from it and
       obtain the value from there.  we also use this to check if
       we have been called from outwith the kernel's code (illegal!)
     */

    addr = ((uint32_t *)regs)[15];
    addr -= 4;
    swi_no = *((unsigned int *)addr) & 0x00ffffff;

    D(bug("[Kernel] ## SWI %d @ 0x%p\n", swi_no, addr));

    if (((char*)addr < &__text_start) || ((char*)addr >= &__text_end))
    {
        D(bug("[Kernel] ## SWI : ILLEGAL ACCESS!\n"));
        return;
    }
    if (swi_no <= 0x0b || swi_no == 0x100)
    {
        DREGS(cpu_DumpRegs(regs));
    
        switch (swi_no)
        {
            case SC_CLI:
            {
                D(bug("[Kernel] ## CLI...\n"));
                ((uint32_t *)regs)[16] |= (1 << 7);
                break;
            }

            case SC_STI:
            {
                D(bug("[Kernel] ## STI...\n"));
                ((uint32_t *)regs)[16] &= ~(1 << 7);
                break;
            }

            case SC_SUPERSTATE:
            {
                D(bug("[Kernel] ## SUPERSTATE... (0x%p ->", ((uint32_t *)regs)[16]));
                ((uint32_t *)regs)[16] &= ~(CPUMODE_MASK);
                ((uint32_t *)regs)[16] |= (0x80 | CPUMODE_SYSTEM);
                D(bug(" 0x%p)\n", ((uint32_t *)regs)[16]));
                break;
            }

            case SC_GETCPUNUMBER:
            {
                ((uint32_t *)regs)[0] = GetCPUNumber();
                break;
            }

            case SC_ISSUPERSTATE:
            {
                D(bug("[Kernel] ## ISSUPERSTATE... "));
                ((uint32_t *)regs)[0] = !(((((uint32_t *)regs)[16] & CPUMODE_MASK) == CPUMODE_USER));
                D(bug("%d\n", ((uint32_t *)regs)[0]));
                break;
            }

            case SC_REBOOT:
            {
                D(bug("[Kernel] ## REBOOT...\n"));
                asm volatile ("mov pc, #0\n"); // Jump to the reset vector..
                break;
            }

            case SC_CACHECLEARE:
            {
                D(bug("[Kernel] ## CACHECLEARE...\n"));
                void * address = ((void **)regs)[0];
                uint32_t length = ((uint32_t *)regs)[1];
                uint32_t flags = ((uint32_t *)regs)[2];

                cache_clear_e(address, length, flags);

                break;
            }

            /*
             * Execure core_SysCall only when we will go back to user mode. Default core_SysCall does
             * not check this condition and could execute Cause() handler before IRQ is entirely handled.
             */

            default:
            {
                uint32_t mode = (((uint32_t *)regs)[16]) & 0x1f;
                if (mode == 0x10 || mode == 0x1f)
                    core_SysCall(swi_no, regs);
                break;
            }
        }
    }
    else
    {
        D(bug("[Kernel] ## SWI : ILLEGAL SWI!\n"));
        return;
    }

    D(bug("[Kernel] ## SWI returning ..\n"));
}
