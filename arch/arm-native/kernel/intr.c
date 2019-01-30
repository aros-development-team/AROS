/*
    Copyright � 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <hardware/intbits.h>
#include <stddef.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_cpu.h"
#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_interrupts.h"
#include "kernel_intr.h"

#define BOOT_STACK_SIZE		(256 << 2)
#define BOOT_TAGS_SIZE          (128 << 3)

#define DREGS(x)
#define DIRQ(x)
#define D(x)

/* linker exports */
extern void *__intvecs_start, *__intvecs_end;
extern void __arm_halt(void);

void ictl_enable_irq(uint8_t irq, struct KernelBase *KernelBase)
{
    if (__arm_arosintern.ARMI_IRQEnable)
        __arm_arosintern.ARMI_IRQEnable(irq);
}

void ictl_disable_irq(uint8_t irq, struct KernelBase *KernelBase)
{
    if (__arm_arosintern.ARMI_IRQDisable)
        __arm_arosintern.ARMI_IRQDisable(irq);
}

asm (
    ".globl __arm_halt                         \n"
    ".type __arm_halt,%function                \n"
    "__arm_halt:                               \n"
    "           b       __arm_halt             \n"
    );

/*
    ** UNDEF INSTRUCTION EXCEPTION
    return addr = lr
    entered in UND mode.
*/

asm (
    ".set	MODE_SYSTEM, 0x1f              \n"

    ".globl __vectorhand_undef                 \n"
    ".type __vectorhand_undef,%function        \n"
    "__vectorhand_undef:                       \n"
    VECTCOMMON_START
    "           bl      handle_undef           \n"
    VECTCOMMON_END
    );

void handle_undef(regs_t *regs)
{
    bug("[Kernel] Trap ARM Undef Exception\n");
    bug("[Kernel]    exception #4 (Illegal instruction)\n");
    bug("[Kernel]    at 0x%p\n", regs->pc);

    if (krnRunExceptionHandlers(KernelBase, 4, regs))
	return;

    D(bug("[Kernel] exception handler(s) returned\n"));

    if (core_Trap(4, regs))
    {
        D(bug("[Kernel] trap handler(s) returned\n"));
        return;
    }

    bug("[Kernel] UNHANDLED EXCEPTION #4\n");

    cpu_DumpRegs(regs);

    __arm_halt();
}

/*
    ** RESET HANDLER
    no return address,
    entered in SVC mode.
*/

asm (
    ".globl __vectorhand_reset                          \n"
    ".type __vectorhand_reset,%function                 \n"
    "__vectorhand_reset:                                \n"
    "           mov     sp, #0x1000 - 16                \n" // re-use bootstrap tmp stack
    "           sub     r0, sp, #" STR(BOOT_STACK_SIZE)"\n" // get the boottag's
    "           sub     r0, r0, #" STR(BOOT_TAGS_SIZE) "\n"
    "           mov     fp, #0                          \n" // clear fp

    "           ldr     pc, 2f                          \n" // jump into kernel resource
    "1:         b       1b                              \n"
    "2:         .word   kernel_cstart                   \n"
    );


/* ** SWI HANDLER ** */

/** SWI handled in syscall.c */

/*
    ** IRQ HANDLER
    return address = lr - 4
    entered in IRQ mode.
*/

asm (
    ".set	MODE_IRQ, 0x12                 \n"
    ".set	MODE_SUPERVISOR, 0x13          \n"
    ".set	MODE_SYSTEM, 0x1f              \n"

    ".globl __vectorhand_irq                   \n"
    ".type __vectorhand_irq,%function          \n"
    "__vectorhand_irq:                         \n"
    "           sub     lr, lr, #4             \n" // adjust lr_irq
    VECTCOMMON_START
    "           bl      handle_irq             \n"
    "           mov     r0, sp                 \n"
    "           ldr     r1, [r0, #16*4]        \n" // load the spr register
    "           and     r1, r1, #31            \n" // mask processor mode
    "           cmp     r1, #0x10              \n" // will we go back to user mode?
    "           cmpne   r1, #0x1f              \n" // or system mode (falls we use it)
    "           bne     1f                     \n" // no? don't call core_ExitInterrupt!
    "           mov     fp, #0                 \n" // clear fp
    "           bl      core_ExitInterrupt     \n"
    "1:                                        \n"
    VECTCOMMON_END
    );

void handle_irq(regs_t *regs)
{
    DIRQ(bug("[Kernel] ## IRQ ##\n"));

    DREGS(cpu_DumpRegs(regs));

    if (__arm_arosintern.ARMI_IRQProcess)
        __arm_arosintern.ARMI_IRQProcess();

    DIRQ(bug("[Kernel] IRQ processing finished\n"));

    return;
}

/*
    ** FIQ HANDLER
    return address = lr -4
    entered in FIQ mode.
*/

__attribute__ ((interrupt ("FIQ"))) void __vectorhand_fiq(void)
{
    DIRQ(bug("[Kernel] ## FIQ ##\n"));

    if (__arm_arosintern.ARMI_FIQProcess)
        __arm_arosintern.ARMI_FIQProcess();

    DIRQ(bug("[Kernel] FIQ processing finished\n"));

    return;
}

/*
    ** DATA ABORT EXCEPTION
    return address = lr - 8
    entered in ABT mode.
*/

asm (
    ".set	MODE_SYSTEM, 0x1f              \n"

    ".globl __vectorhand_dataabort             \n"
    ".type __vectorhand_dataabort,%function    \n"
    "__vectorhand_dataabort:                   \n"
    "           sub     lr, lr, #8             \n" // adjust lr_irq
    VECTCOMMON_START
    "           bl      handle_dataabort       \n"
    VECTCOMMON_END
    );

void handle_dataabort(regs_t *regs)
{
    register unsigned int far;

    // Read fault address register
    asm volatile("mrc p15, 0, %[far], c6, c0, 0": [far] "=r" (far) );

    bug("[Kernel] Trap ARM Data Abort Exception\n");
    bug("[Kernel]    exception #2 (Bus Error)\n");
    bug("[Kernel]    attempt to access 0x%p from 0x%p\n", far, regs->pc);

    cpu_DumpRegs(regs);

    if (krnRunExceptionHandlers(KernelBase, 2, regs))
	return;

    D(bug("[Kernel] exception handler(s) returned\n"));

    if (core_Trap(2, regs))
    {
        D(bug("[Kernel] trap handler(s) returned\n"));
        return;
    }

    bug("[Kernel] UNHANDLED EXCEPTION #2\n");

    cpu_DumpRegs(regs);

    __arm_halt();
}

/*
    ** PREFETCH ABORT EXCEPTION
    return address = lr - 4
    entered in ABT mode.
*/

asm (
    ".set	MODE_SYSTEM, 0x1f              \n"

    ".globl __vectorhand_prefetchabort         \n"
    ".type __vectorhand_prefetchabort,%function \n"
    "__vectorhand_prefetchabort:               \n"
    "           sub     lr, lr, #4             \n" // adjust lr_irq
    VECTCOMMON_START

    "           bl      handle_prefetchabort   \n"

    VECTCOMMON_END
    );

void handle_prefetchabort(regs_t *regs)
{
    bug("[Kernel] Trap ARM Prefetch Abort Exception\n");
    bug("[Kernel]    exception #3 (Address Error)\n");
    bug("[Kernel]    at 0x%p\n", regs->pc);

    cpu_DumpRegs(regs);

    if (krnRunExceptionHandlers(KernelBase, 3, regs))
	return;

    D(bug("[Kernel] exception handler(s) returned\n"));

    if (core_Trap(3, regs))
    {
        D(bug("[Kernel] trap handler(s) returned\n"));
        return;
    }

    bug("[Kernel] UNHANDLED EXCEPTION #3\n");

    cpu_DumpRegs(regs);

    __arm_halt();
}


/* ** SETUP ** */

void arm_flush_cache(uint32_t addr, uint32_t length)
{
    while (length)
    {
        __asm__ __volatile__("mcr p15, 0, %0, c7, c14, 1"::"r"(addr));
        addr += 32;
        length -= 32;
    }
    __asm__ __volatile__("mcr p15, 0, %0, c7, c10, 4"::"r"(addr));
}

void arm_icache_invalidate(uint32_t addr, uint32_t length)
{
    while (length)
    {
        __asm__ __volatile__("mcr p15, 0, %0, c7, c5, 1"::"r"(addr));
        addr += 32;
        length -= 32;
    }
    __asm__ __volatile__("mcr p15, 0, %0, c7, c10, 4"::"r"(addr));
}

/*
    Beware! Here be dragons!

    This function copies our vector table to address 0. We do that in *two* steps:
    Fist we copy four bytes from source to address 0 using ASM, because gcc will
    not allow to dereference null pointer. THen the rest of the table is copied.
*/
void copy_vectors(const void * src, int length)
{
    char *d = (char *)4;
    const char *s = (const char *)src + 4;

    asm volatile("str %1, [%0]"::"r"(0), "r"(*(ULONG*)src));

    while(length--)
    {
        *d++ = *s++;
    }
}

void core_SetupIntr(void)
{
    bug("[Kernel] Initializing cpu vectors\n");

    /* Copy vectors into place */
    copy_vectors(&__intvecs_start,
            (unsigned int)&__intvecs_end -
            (unsigned int)&__intvecs_start);

    arm_flush_cache(0, 1024);
    arm_icache_invalidate(0, 1024);

    D(bug("[Kernel] Copied %d bytes from 0x%p to 0x00000000\n", (unsigned int)&__intvecs_end - (unsigned int)&__intvecs_start, &__intvecs_start));

    D(
        unsigned int x = 0;
        bug("[Kernel]: Vector dump-:");
        for (x=0; x < (unsigned int)&__intvecs_end - (unsigned int)&__intvecs_start; x++) {
            if ((x%16) == 0)
            {
                bug("\n[Kernel]:     %08x:", x);
            }
            bug(" %02x", *((volatile UBYTE *)x));
        }
        bug("\n");
    )

    if (__arm_arosintern.ARMI_IRQInit)
        __arm_arosintern.ARMI_IRQInit();
}
