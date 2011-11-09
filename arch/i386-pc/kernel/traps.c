/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <asm/io.h>
#include <proto/exec.h>

#include "cpu_traps.h"
#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_globals.h"
#include "kernel_intern.h"
#include "kernel_interrupts.h"
#include "kernel_intr.h"
#include "kernel_syscall.h"
#include "apic.h"
#include "traps.h"
#include "xtpic.h"

#define D(x)
#define DSYSCALL(x)

/* 0,1,5-7,9-17,19:
		return address of these exceptions is the address of faulting instr
   1,3,4:
		return address is address of instruction followed by trapping instr
		(1 can be FAULT and TRAP)
	others:
		ABORT = ??? (no information = no return address)
*/

BUILD_TRAP(0x00)
BUILD_TRAP(0x01)
BUILD_TRAP(0x02)
BUILD_TRAP(0x03)
BUILD_TRAP(0x04)
BUILD_TRAP(0x05)
BUILD_TRAP(0x06)
BUILD_TRAP(0x07)
BUILD_TRAP(0x08)
BUILD_TRAP(0x09)
BUILD_TRAP(0x0a)
BUILD_TRAP(0x0b)
BUILD_TRAP(0x0c)
BUILD_TRAP(0x0d)
BUILD_TRAP(0x0e)
BUILD_TRAP(0x0f)
BUILD_TRAP(0x10)
BUILD_TRAP(0x11)
BUILD_TRAP(0x12)
BUILD_TRAP(0x13)
BUILD_TRAP(0x14)
BUILD_TRAP(0x15)
BUILD_TRAP(0x16)
BUILD_TRAP(0x17)
BUILD_TRAP(0x18)
BUILD_TRAP(0x19)
BUILD_TRAP(0x1a)
BUILD_TRAP(0x1b)
BUILD_TRAP(0x1c)
BUILD_TRAP(0x1d)
BUILD_TRAP(0x1e)
BUILD_TRAP(0x1f)
BUILD_TRAP(0x20)
BUILD_TRAP(0x21)
BUILD_TRAP(0x22)
BUILD_TRAP(0x23)
BUILD_TRAP(0x24)
BUILD_TRAP(0x25)
BUILD_TRAP(0x26)
BUILD_TRAP(0x27)
BUILD_TRAP(0x28)
BUILD_TRAP(0x29)
BUILD_TRAP(0x2a)
BUILD_TRAP(0x2b)
BUILD_TRAP(0x2c)
BUILD_TRAP(0x2d)
BUILD_TRAP(0x2e)
BUILD_TRAP(0x2f)

BUILD_TRAP(0x80)
BUILD_TRAP(0xFE)

typedef void (*trap_type)(void);

const trap_type traps[48] =
{
	TRAP0x00_trap,
	TRAP0x01_trap,
	TRAP0x02_trap,
	TRAP0x03_trap,
	TRAP0x04_trap,
	TRAP0x05_trap,
	TRAP0x06_trap,
	TRAP0x07_trap,
	TRAP0x08_trap,
	TRAP0x09_trap,
	TRAP0x0a_trap,
	TRAP0x0b_trap,
	TRAP0x0c_trap,
	TRAP0x0d_trap,
	TRAP0x0e_trap,
	TRAP0x0f_trap,
	TRAP0x10_trap,
	TRAP0x11_trap,
	TRAP0x12_trap,
	TRAP0x13_trap,
	TRAP0x14_trap,
	TRAP0x15_trap,
	TRAP0x16_trap,
	TRAP0x17_trap,
	TRAP0x18_trap,
	TRAP0x19_trap,
	TRAP0x1a_trap,
	TRAP0x1b_trap,
	TRAP0x1c_trap,
	TRAP0x1d_trap,
	TRAP0x1e_trap,
	TRAP0x1f_trap,
	TRAP0x20_trap,
	TRAP0x21_trap,
	TRAP0x22_trap,
	TRAP0x23_trap,
	TRAP0x24_trap,
	TRAP0x25_trap,
	TRAP0x26_trap,
	TRAP0x27_trap,
	TRAP0x28_trap,
	TRAP0x29_trap,
	TRAP0x2a_trap,
	TRAP0x2b_trap,
	TRAP0x2c_trap,
	TRAP0x2d_trap,
	TRAP0x2e_trap,
	TRAP0x2f_trap
};

#define _set_gate(gate_addr,type,dpl,addr) \
do { \
  int __d0, __d1; \
  __asm__ __volatile__ ("movw %%dx,%%ax\n\t" \
	"movw %4,%%dx\n\t" \
	"movl %%eax,%0\n\t" \
	"movl %%edx,%1" \
	:"=m" (*((long *) (gate_addr))), \
	 "=m" (*(1+(long *) (gate_addr))), "=&a" (__d0), "=&d" (__d1) \
	:"i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
	 "3" ((char *) (addr)),"2" (KERNEL_CS << 16)); \
} while (0)

void set_intr_gate(unsigned int n, void *addr)
{
    struct PlatformData *data = KernelBase->kb_PlatformData;
 
    _set_gate(&data->idt[n], 14, 0, addr);
}

void set_system_gate(unsigned int n, void *addr)
{
    struct PlatformData *data = KernelBase->kb_PlatformData;

    _set_gate(&data->idt[n], 14, 3, addr);
}

void handleException(struct ExceptionContext *regs, unsigned long error_code, unsigned long irq_number)
{
    struct KernelBase *KernelBase = getKernelBase();

    if (irq_number < 0x20)
    {
	/* These are CPU traps */
	cpu_Trap(regs, error_code, irq_number);
    }
    else if ((irq_number >= 0x20) && (irq_number < 0x30)) /* XT-PIC IRQ */
    {
	/* From CPU's point of view, IRQs are exceptions starting from 0x20. */
    	irq_number -= 0x20;

	if (irq_number == 13)
	{
	    /* FPU error IRQ */
	    outb(0, 0xF0);
	}

	if (KernelBase)
    	{
            XTPIC_AckIntr(irq_number, &KernelBase->kb_PlatformData->xtpic_mask);
	    krnRunIRQHandlers(KernelBase, irq_number);

	    /*
	     * Interrupt acknowledge on XT-PIC also disables this interrupt.
	     * If we still need it, we need to re-enable it.
	     */
            if (!IsListEmpty(&KernelBase->kb_Interrupts[irq_number]))
	    {
		XTPIC_EnableIRQ(irq_number, &KernelBase->kb_PlatformData->xtpic_mask);
	    }
	}

	/* Upon exit from the lowest-level hardware IRQ we run the task scheduler */
	if (SysBase && (regs->ds != KERNEL_DS))
	{
	    /* Disable interrupts for a while */
	    __asm__ __volatile__("cli; cld;");

	    core_ExitInterrupt(regs);
	}
    }
    else if (irq_number == 0x80)  /* Syscall? */
    {
	/* Syscall number is actually ULONG (we use only eax) */
    	ULONG sc = regs->eax;

        DSYSCALL(bug("[Kernel] Syscall %u\n", sc));

	/* The following syscalls can be run in both supervisor and user mode */
	switch (sc)
	{
	case SC_REBOOT:
	    D(bug("[Kernel] Warm restart\n"));
	    core_Reboot();

	case SC_SUPERVISOR:
	    /* This doesn't return */
	    core_Supervisor(regs);
	}

	/*
	 * Scheduler can be called only from within user mode.
	 * Every task has ss register initialized to a valid segment descriptor.\
	 * The descriptor itself isn't used by x86-64, however when a privilege
	 * level switch occurs upon an interrupt, ss is reset to zero. Old ss value
	 * is always pushed to stack as part of interrupt context.
	 * We rely on this in order to determine which CPL we are returning to.
	 */
        if (regs->ds != KERNEL_DS)
        {
            DSYSCALL(bug("[Kernel] User-mode syscall\n"));

	    /* Disable interrupts for a while */
	    __asm__ __volatile__("cli; cld;");

	    core_SysCall(sc, regs);
        }

	DSYSCALL(bug("[Kernel] Returning from syscall...\n"));
    }
    else if (irq_number == 0xFE)
    {
	/* APIC error vector */
        core_APIC_AckIntr();
    }
    /* Return from this routine is equal to core_LeaveInterrupt(regs) */
}

void Init_Traps(struct PlatformData *data)
{
    int i;

    for (i = 0; i < 0x30; i++)
    {
	_set_gate(&data->idt[i], 14, 0, traps[i]);
    }
    /* Set all unused vectors to dummy interrupt */
    for (i = 0x30; i < 256; i++)
    {
	_set_gate(&data->idt[i], 14, 0, core_Unused_Int);
    }

    /* Create user interrupt used to enter supervisor mode */
    _set_gate(&data->idt[0x80], 14, 3, TRAP0x80_trap);
    /* Create APIC error vector */
    _set_gate(&data->idt[0xFE], 14, 0, TRAP0xFE_trap);
}
