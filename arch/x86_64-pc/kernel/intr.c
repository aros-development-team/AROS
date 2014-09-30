/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
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
#include "kernel_bootmem.h"
#include "kernel_debug.h"
#include "kernel_globals.h"
#include "kernel_interrupts.h"
#include "kernel_intern.h"
#include "kernel_intr.h"
#include "kernel_scheduler.h"
#include "kernel_syscall.h"
#include "cpu_traps.h"
#include "apic.h"
#include "xtpic.h"

#define D(x)
#define DSYSCALL(x)
#define DTRAP(x)
#define DUMP_CONTEXT

/*
 * Simulate SysBase access at address 8.
 * Disabled because global SysBase is moved away from zeropage.
 *
#define EMULATE_SYSBASE 8 */

#define IRQ(x,y) \
    IRQ##x##y##_intr

#define IRQPROTO(x, y) \
    void IRQ(x, y)(void)

#define IRQPROTO_16(x) \
    IRQPROTO(x,0); IRQPROTO(x,1); IRQPROTO(x,2); IRQPROTO(x,3); \
    IRQPROTO(x,4); IRQPROTO(x,5); IRQPROTO(x,6); IRQPROTO(x,7); \
    IRQPROTO(x,8); IRQPROTO(x,9); IRQPROTO(x,a); IRQPROTO(x,b); \
    IRQPROTO(x,c); IRQPROTO(x,d); IRQPROTO(x,e); IRQPROTO(x,f)

#define IRQLIST_16(x) \
    IRQ(x,0), IRQ(x,1), IRQ(x,2), IRQ(x,3), \
    IRQ(x,4), IRQ(x,5), IRQ(x,6), IRQ(x,7), \
    IRQ(x,8), IRQ(x,9), IRQ(x,a), IRQ(x,b), \
    IRQ(x,c), IRQ(x,d), IRQ(x,e), IRQ(x,f)

/* This generates prototypes for entry points */
IRQPROTO_16(0x0);
IRQPROTO_16(0x1);
IRQPROTO_16(0x2);
IRQPROTO(0x8, 0);
IRQPROTO(0xf, e);
extern void core_DefaultIRETQ(void);

const void *interrupt[256] =
{
    IRQLIST_16(0x0),
    IRQLIST_16(0x1),
    IRQLIST_16(0x2)
};

void core_SetupIDT(struct KernBootPrivate *__KernBootPrivate)
{
    int i;
    uintptr_t off;
    struct segment_selector IDT_sel;
    struct int_gate_64bit *IGATES;

    if (!__KernBootPrivate->IDT)
    {
    	__KernBootPrivate->IDT = krnAllocBootMemAligned(sizeof(struct int_gate_64bit) * 256, 256);

    	D(bug("[Kernel] Allocated IDT at 0x%p\n", __KernBootPrivate->IDT));
    }

    D(bug("[Kernel] core_SetupIDT: Setting all interrupt handlers to default value\n"));
    IGATES = __KernBootPrivate->IDT;

    for (i=0; i < 256; i++)
    {
        if (interrupt[i])
            off = (uintptr_t)interrupt[i];
        else if (i == 0x80)
            off = (uintptr_t)IRQ0x80_intr;
        else if (i == 0xfe)
            off = (uintptr_t)IRQ0xfe_intr;
        else
            off = (uintptr_t)core_DefaultIRETQ;

        IGATES[i].offset_low = off & 0xffff;
        IGATES[i].offset_mid = (off >> 16) & 0xffff;
        IGATES[i].offset_high = (off >> 32) & 0xffffffff;
        IGATES[i].type = 0x0e;
        IGATES[i].dpl = 3;
        IGATES[i].p = 1;
        IGATES[i].selector = KERNEL_CS;
        IGATES[i].ist = 0;
    }

    D(bug("[Kernel] core_SetupIDT: Registering interrupt handlers ..\n"));

    IDT_sel.size = sizeof(struct int_gate_64bit) * 256 - 1;
    IDT_sel.base = (unsigned long)__KernBootPrivate->IDT;    
    asm volatile ("lidt %0"::"m"(IDT_sel));
}

/* CPU exceptions are processed here */
void core_IRQHandle(struct ExceptionContext *regs, unsigned long error_code, unsigned long irq_number)
{
    struct KernelBase *KernelBase = getKernelBase();

#ifdef EMULATE_SYSBASE
    if (irq_number == 0x0e)
    {
        uint64_t ptr = rdcr(cr2);
	unsigned char *ip = (unsigned char *)regs->rip;

	D(bug("[Kernel] Page fault exception\n"));

        if (ptr == EMULATE_SYSBASE)
        {
            D(bug("[Kernel] ** Code at 0x%p is trying to access the SysBase at 0x%p.\n", ip, ptr));

            if ((ip[0] & 0xfb) == 0x48 &&
                 ip[1]         == 0x8b && 
                (ip[2] & 0xc7) == 0x04 &&
                 ip[3]         == 0x25)
            {
                int reg = ((ip[2] >> 3) & 0x07) | ((ip[0] & 0x04) << 1);

                switch(reg)
                {
                    case 0:
                        regs->rax = (UQUAD)SysBase;
                        break;
                    case 1:
                        regs->rcx = (UQUAD)SysBase;
                        break;
                    case 2:
                        regs->rdx = (UQUAD)SysBase;
                        break;
                    case 3:
                        regs->rbx = (UQUAD)SysBase;
                        break;
//                    case 4:   /* Cannot put SysBase into rSP register */
//                        regs->rsp = (UQUAD)SysBase;
//                        break;
                    case 5:
                        regs->rbp = (UQUAD)SysBase;
                        break;
                    case 6:
                        regs->rsi = (UQUAD)SysBase;
                        break;
                    case 7:
                        regs->rdi = (UQUAD)SysBase;
                        break;
                    case 8:
                        regs->r8 = (UQUAD)SysBase;
                        break;
                    case 9:
                        regs->r9 = (UQUAD)SysBase;
                        break;
                    case 10:
                        regs->r10 = (UQUAD)SysBase;
                        break;
                    case 11:
                        regs->r11 = (UQUAD)SysBase;
                        break;
                    case 12:
                        regs->r12 = (UQUAD)SysBase;
                        break;
                    case 13:
                        regs->r13 = (UQUAD)SysBase;
                        break;
                    case 14:
                        regs->r14 = (UQUAD)SysBase;
                        break;
                    case 15:
                        regs->r15 = (UQUAD)SysBase;
                        break;
                }

                regs->rip += 8;
                
                core_LeaveInterrupt(regs);
            }
            else if ((ip[0] & 0xfb) == 0x48 &&
                      ip[1]         == 0x8b && 
                     (ip[2] & 0xc7) == 0x05)
            {
                int reg = ((ip[2] >> 3) & 0x07) | ((ip[0] & 0x04) << 1);

                switch(reg)
                {
                    case 0:
                        regs->rax = (UQUAD)SysBase;
                        break;
                    case 1:
                        regs->rcx = (UQUAD)SysBase;
                        break;
                    case 2:
                        regs->rdx = (UQUAD)SysBase;
                        break;
                    case 3:
                        regs->rbx = (UQUAD)SysBase;
                        break;
//                    case 4:   /* Cannot put SysBase into rSP register */
//                        regs->rsp = (UQUAD)SysBase;
//                        break;
                    case 5:
                        regs->rbp = (UQUAD)SysBase;
                        break;
                    case 6:
                        regs->rsi = (UQUAD)SysBase;
                        break;
                    case 7:
                        regs->rdi = (UQUAD)SysBase;
                        break;
                    case 8:
                        regs->r8 = (UQUAD)SysBase;
                        break;
                    case 9:
                        regs->r9 = (UQUAD)SysBase;
                        break;
                    case 10:
                        regs->r10 = (UQUAD)SysBase;
                        break;
                    case 11:
                        regs->r11 = (UQUAD)SysBase;
                        break;
                    case 12:
                        regs->r12 = (UQUAD)SysBase;
                        break;
                    case 13:
                        regs->r13 = (UQUAD)SysBase;
                        break;
                    case 14:
                        regs->r14 = (UQUAD)SysBase;
                        break;
                    case 15:
                        regs->r15 = (UQUAD)SysBase;
                        break;
                }
                
                regs->rip += 7;
                
                core_LeaveInterrupt(regs);
            }
                D(else bug("[Kernel] Instruction not recognized\n"));
        }

#ifdef DUMP_CONTEXT
	unsigned int i;

        bug("[Kernel] PAGE FAULT accessing 0x%p\n", ptr);
        bug("[Kernel] Insn: ");
        for (i = 0; i < 16; i++)
            bug("%02x ", ip[i]);
        bug("\n");
#endif

	/* The exception will now be passed on to handling code below */
    }
#endif

    /* These exceptions are CPU traps */
    if (irq_number < 0x20)
    {
    	cpu_Trap(regs, error_code, irq_number);
    }
    else if (irq_number == 0x80)  /* Syscall? */
    {
	/* Syscall number is actually ULONG (we use only eax) */
    	ULONG sc = regs->rax;

        DSYSCALL(bug("[Kernel] Syscall %u\n", sc));

	/* The following syscalls can be run in both supervisor and user mode */
	switch (sc)
	{
	case SC_REBOOT:
	    D(bug("[Kernel] Warm restart, stack 0x%p\n", AROS_GET_SP));

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
	    __asm__ __volatile__(
	    	"cli\n\t"
	    	"cld\n\t"
	    	"movq %0, %%rsp\n\t"
	    	"jmp *%1\n"
	    	::"r"(__KernBootPrivate->SystemStack + STACK_SIZE), "r"(core_Kick), "D"(BootMsg), "S"(kernel_cstart));

	case SC_SUPERVISOR:
	    /* This doesn't return */
	    core_Supervisor(regs);
	}

	/*
	 * Scheduler can be called only from within user mode.
	 * Every task has ss register initialized to a valid segment descriptor.
	 * The descriptor itself isn't used by x86-64, however when a privilege
	 * level switch occurs upon an interrupt, ss is reset to zero. Old ss value
	 * is always pushed to stack as part of interrupt context.
	 * We rely on this in order to determine which CPL we are returning to.
	 */
        if (regs->ss != 0)
        {
            DSYSCALL(bug("[Kernel] User-mode syscall\n"));

	    /* Disable interrupts for a while */
	    __asm__ __volatile__("cli; cld;");

	    core_SysCall(sc, regs);
        }

	DSYSCALL(bug("[Kernel] Returning from syscall...\n"));
    }
    else if (irq_number >= 0x20) /* Hardware IRQ */
    {
	if (KernelBase)
    	{
	    /* From CPU's point of view, IRQs are exceptions starting from 0x20. */
    	    irq_number -= 0x20;

    	    switch (KernelBase->kb_Interrupts[irq_number].lh_Type)
    	    {
    	    case KBL_APIC:
            	core_APIC_AckIntr();
            	break;

            case KBL_XTPIC:
            	XTPIC_AckIntr(irq_number, &KernelBase->kb_PlatformData->kb_XTPIC_Mask);
	 	krnRunIRQHandlers(KernelBase, irq_number);

		/*
		 * Interrupt acknowledge on XT-PIC also disables this interrupt.
		 * If we still need it, we need to re-enable it.
		 */
            	if (!IsListEmpty(&KernelBase->kb_Interrupts[irq_number]))
                    XTPIC_EnableIRQ(irq_number, &KernelBase->kb_PlatformData->kb_XTPIC_Mask);

                break;
	    }
	}

	/* Upon exit from the lowest-level hardware IRQ we run the task scheduler */
	if (SysBase && (regs->ss != 0))
	{
	    /* Disable interrupts for a while */
	    __asm__ __volatile__("cli; cld;");

	    core_ExitInterrupt(regs);
	}
    }

    core_LeaveInterrupt(regs);
}

void ictl_enable_irq(unsigned char irq, struct KernelBase *KernelBase)
{
    if (KernelBase->kb_Interrupts[irq].lh_Type == KBL_XTPIC)
        XTPIC_EnableIRQ(irq, &KernelBase->kb_PlatformData->kb_XTPIC_Mask);
}

void ictl_Initialize(void)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct PlatformData *pdata = KernelBase->kb_PlatformData;

    if (!pdata->kb_APIC)
    {
	/* No APIC was discovered by ACPI/whatever else. Do the probe. */
	pdata->kb_APIC = core_APIC_Probe();
    }

    if (!pdata)
    {
    	/* We are x86-64 and we always have APIC. */
    	krnPanic(KernelBase, "Failed to allocate APIC descriptor\n.The system is low on memory.");
    }

    if (pdata->kb_APIC->flags & APF_8259)
    {
    	/*
    	 * Initialize legacy 8529A PIC.
    	 * TODO: We obey ACPI information about its presence, however currently we don't have
    	 * IOAPIC support. Switching to IOAPIC requires full ACPI support including AML.
    	 */

    	XTPIC_Init(&pdata->kb_XTPIC_Mask);
    }

    D(bug("[Kernel] kernel_cstart: Interrupts redirected. We will go back in a minute ;)\n"));
}
