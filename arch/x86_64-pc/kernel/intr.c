/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
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

#define D(x)
#define DIDT(x)
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

BOOL core_SetIDTGate(struct int_gate_64bit *IGATES, int IRQ, uintptr_t gate)
{
    DIDT(
        bug("[Kernel] %s: Setting IRQ #%d gate for IDT @ 0x%p\n", __func__, IRQ, IGATES);
        bug("[Kernel] %s: gate @ 0x%p\n", __func__, gate);
    )
    IGATES[IRQ].offset_low = gate & 0xffff;
    IGATES[IRQ].offset_mid = (gate >> 16) & 0xffff;
    IGATES[IRQ].offset_high = (gate >> 32) & 0xffffffff;
    IGATES[IRQ].type = 0x0e;
    IGATES[IRQ].dpl = 3;
    IGATES[IRQ].p = 1;
    IGATES[IRQ].selector = KERNEL_CS;
    IGATES[IRQ].ist = 0;

    return TRUE;
}

void core_SetupIDT(struct KernBootPrivate *__KernBootPrivate, apicid_t _APICID, APTR idt_ptr)
{
    int i;
    uintptr_t off;
    struct segment_selector IDT_sel;
    struct int_gate_64bit *IGATES = (struct int_gate_64bit *)idt_ptr;

    // TODO: ASSERT IGATES is aligned
    
    if (IGATES)
    {
        DIDT(
            bug("[Kernel] %s[%d]: IDT @ 0x%p\n", __func__, _APICID, IGATES);
            bug("[Kernel] %s[%d]: Setting default gates\n", __func__, _APICID);
        )

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

            if (!core_SetIDTGate(IGATES, i, off))
            {
                bug("[Kernel] %s[%d]: gate #%d failed\n", __func__, _APICID, i);
            }
        }

        DIDT(bug("[Kernel] %s[%d]: Registering IDT ..\n", __func__, _APICID));

        IDT_sel.size = sizeof(struct int_gate_64bit) * 256 - 1;
        IDT_sel.base = (unsigned long)IGATES;
        DIDT(bug("[Kernel] %s[%d]:    base 0x%p, size %d\n", __func__, _APICID, IDT_sel.base, IDT_sel.size));

        asm volatile ("lidt %0"::"m"(IDT_sel));
    }
    else
    {
        krnPanic(NULL, "Invalid IDT\n");
    }
    DIDT(bug("[Kernel] %s[%d]: IDT configured\n", __func__, _APICID));
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

    /* The first 32 "hardware IRQs" are CPU exceptions */
    if (irq_number < HW_IRQ_BASE)
    {
    	cpu_Trap(regs, error_code, irq_number);
    }
    else if (irq_number == 0x80)  /* Syscall? */
    {
        struct PlatformData *pdata = KernelBase->kb_PlatformData;
        struct syscallx86_Handler *scHandler;
    	ULONG sc = regs->rax;

	/* Syscall number is actually ULONG (we use only eax) */
        DSYSCALL(bug("[Kernel] Syscall %08x\n", sc));

        ForeachNode(&pdata->kb_SysCallHandlers, scHandler)
        {
            if ((ULONG)((IPTR)scHandler->sc_Node.ln_Name) == sc)
            {
                scHandler->sc_SysCall(regs);
            }
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
    else if (irq_number >= HW_IRQ_BASE) /* Hardware IRQ */
    {
	if (KernelBase)
    	{
            struct IntrController *irqIC;

	    /* From CPU's point of view, IRQs are exceptions starting from 0x20. */
    	    irq_number -= HW_IRQ_BASE;

            if ((irqIC = krnGetInterruptController(KernelBase, KernelBase->kb_Interrupts[irq_number].lh_Type)) != NULL)
            {
                if (irqIC->ic_IntrAck)
                    irqIC->ic_IntrAck(irqIC->ic_Private, KernelBase->kb_Interrupts[irq_number].l_pad, irq_number);

	 	krnRunIRQHandlers(KernelBase, irq_number);

                if ((irqIC->ic_Flags & ICF_ACKENABLE) &&
                    (irqIC->ic_IntrEnable) &&
                    (!IsListEmpty(&KernelBase->kb_Interrupts[irq_number])))
                    irqIC->ic_IntrEnable(irqIC->ic_Private, KernelBase->kb_Interrupts[irq_number].l_pad, irq_number);
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
