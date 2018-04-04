/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
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
#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_globals.h"
#include "kernel_interrupts.h"
#include "kernel_intr.h"
#include "kernel_scheduler.h"
#include "kernel_syscall.h"
#include "kernel_ipi.h"
#include "cpu_traps.h"

#define D(x)
#define DIDT(x)
#define DIRQ(x)
#define DTRAP(x)
#define DUMP_CONTEXT

//#define IRQNOSCHED_FORBID

/* use the correct registers depending on arch. */
#if (__WORDSIZE != 64)
#define INTR_REGA               regs->eax
#else
#define INTR_REGA               regs->rax
#endif

#define IRQ(x,y) \
    IRQ##x##y##_intr

#define IRQPROTO(x, y) \
    void IRQ(x, y)(void)

#define IRQPROTO_16(x) \
    IRQPROTO(x,0); IRQPROTO(x,1); IRQPROTO(x,2); IRQPROTO(x,3); \
    IRQPROTO(x,4); IRQPROTO(x,5); IRQPROTO(x,6); IRQPROTO(x,7); \
    IRQPROTO(x,8); IRQPROTO(x,9); IRQPROTO(x,A); IRQPROTO(x,B); \
    IRQPROTO(x,C); IRQPROTO(x,D); IRQPROTO(x,E); IRQPROTO(x,F)

#define IRQLIST_16(x) \
    IRQ(x,0), IRQ(x,1), IRQ(x,2), IRQ(x,3), \
    IRQ(x,4), IRQ(x,5), IRQ(x,6), IRQ(x,7), \
    IRQ(x,8), IRQ(x,9), IRQ(x,A), IRQ(x,B), \
    IRQ(x,C), IRQ(x,D), IRQ(x,E), IRQ(x,F)

/* This generates prototypes for entry points */
IRQPROTO_16(0x0);
IRQPROTO_16(0x1);
IRQPROTO_16(0x2);
IRQPROTO_16(0x3);
IRQPROTO_16(0x4);
IRQPROTO_16(0x5);
IRQPROTO_16(0x6);
IRQPROTO_16(0x7);
IRQPROTO_16(0x8);
IRQPROTO_16(0x9);
IRQPROTO_16(0xA);
IRQPROTO_16(0xB);
IRQPROTO_16(0xC);
IRQPROTO_16(0xD);
IRQPROTO_16(0xE);
IRQPROTO_16(0xF);
extern void DEF_IRQRETFUNC(void);

const void *IntrDefaultGates[256] =
{
    IRQLIST_16(0x0),
    IRQLIST_16(0x1),
    IRQLIST_16(0x2),
    IRQLIST_16(0x3),
    IRQLIST_16(0x4),
    IRQLIST_16(0x5),
    IRQLIST_16(0x6),
    IRQLIST_16(0x7),
    IRQLIST_16(0x8),
    IRQLIST_16(0x9),
    IRQLIST_16(0xA),
    IRQLIST_16(0xB),
    IRQLIST_16(0xC),
    IRQLIST_16(0xD),
    IRQLIST_16(0xE),
    IRQLIST_16(0xF)
};

/* Set the raw CPU vectors gate in the IDT */
BOOL core_SetIDTGate(apicidt_t *IGATES, int vect, uintptr_t gate, BOOL enable)
{
    DIDT(
        APTR gateOld;

        bug("[Kernel] %s: Setting IDTGate #%d IDT @ 0x%p\n", __func__, vect, IGATES);
        bug("[Kernel] %s: gate @ 0x%p\n", __func__, gate);
        bug("[Kernel] %s: enable=%d\n", __func__, enable);

        gateOld = 
#if (__WORDSIZE != 64)
            (APTR)((((IPTR)IGATES[vect].offset_high & 0xFFFF) << 16) | ((IPTR)IGATES[vect].offset_low & 0xFFFF));
#else
            (APTR)((((IPTR)IGATES[vect].offset_high & 0xFFFFFFFF) << 32) | (((IPTR)IGATES[vect].offset_mid & 0xFFFF) << 16) | ((IPTR)IGATES[vect].offset_low & 0xFFFF));
#endif
        if (gateOld) bug("[Kernel] %s: existing gate @ 0x%p\n", __func__, gateOld);
    )

    /* If the gate isn't already enabled, set it */
    if (!IGATES[vect].p)
    {
        IGATES[vect].offset_low = gate & 0xFFFF;
#if (__WORDSIZE != 64)
        IGATES[vect].offset_high = (gate >> 16) & 0xFFFF;
#else
        IGATES[vect].offset_mid = (gate >> 16) & 0xFFFF;
        IGATES[vect].offset_high = (gate >> 32) & 0xFFFFFFFF;
#endif
        IGATES[vect].type = 0x0E;
        IGATES[vect].dpl = 3;
        if (enable)
            IGATES[vect].p = 1;
        IGATES[vect].selector = KERNEL_CS;
        IGATES[vect].ist = 0;

        return TRUE;
    }
    else
    {
        bug("[Kernel] %s: Vector #%d gate already enabled!\n", __func__, vect);
    }
    return FALSE;
}

/* Set a hardware IRQ's gate in the IDT */
BOOL core_SetIRQGate(void *idt, int IRQ, uintptr_t gate)
{
    apicidt_t *IGATES = (apicidt_t *)idt;
    DIDT(
        bug("[Kernel] %s: Setting IRQGate #%d\n", __func__, IRQ);
        bug("[Kernel] %s: gate @ 0x%p\n", __func__, gate);
    )

    return core_SetIDTGate(IGATES, HW_IRQ_BASE + IRQ, gate, TRUE);
}

void core_ReloadIDT()
{
    struct KernelBase *KernelBase = getKernelBase();
    struct APICData *apicData  = KernelBase->kb_PlatformData->kb_APIC;
    apicid_t cpuNo = KrnGetCPUNumber();
    struct segment_selector IDT_sel;

    apicidt_t *IGATES = (apicidt_t *)apicData->cores[cpuNo].cpu_IDT;

    DIRQ(bug("[Kernel] %s()\n", __func__);)

    IDT_sel.size = sizeof(apicidt_t) * 256 - 1;
    IDT_sel.base = (unsigned long)IGATES;
    DIDT(bug("[Kernel] %s[%d]:    base 0x%p, size %d\n", __func__, cpuNo, IDT_sel.base, IDT_sel.size));

    asm volatile ("lidt %0"::"m"(IDT_sel));
}

void core_SetupIDT(apicid_t _APICID, apicidt_t *IGATES)
{
    int i;
    uintptr_t off;
    struct segment_selector IDT_sel;

    // TODO: ASSERT IGATES is aligned
    
    if (IGATES)
    {
        DIDT(
            bug("[Kernel] %s[%d]: IDT @ 0x%p\n", __func__, _APICID, IGATES);
            bug("[Kernel] %s[%d]: Setting default gates\n", __func__, _APICID);
        )

        // Disable ALL the default gates until something takes ownership
        for (i=0; i < 256; i++)
        {
            off = (uintptr_t)DEF_IRQRETFUNC;

            if (!core_SetIDTGate(IGATES, i, off, FALSE))
            {
                bug("[Kernel] %s[%d]: gate #%d failed\n", __func__, _APICID, i);
            }
        }

        DIDT(bug("[Kernel] %s[%d]: Registering IDT ..\n", __func__, _APICID));

        IDT_sel.size = sizeof(apicidt_t) * 256 - 1;
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
void core_IRQHandle(struct ExceptionContext *regs, unsigned long error_code, unsigned long int_number)
{
    struct KernelBase *KernelBase = getKernelBase();
#if 0 
    // This debug works only if Local APIC exists...
    DIRQ(
        IPTR __APICBase = core_APIC_GetBase();
        int cpunum = KrnGetCPUNumber();

        bug("core_IRQHandle(%d), eflags=%08x\n", int_number, regs->rflags);

        bug("IRR.%03x: %08x%08x%08x%08x%08x%08x%08x%08x\n", cpunum,
            APIC_REG(__APICBase, APIC_IRR+0x70), APIC_REG(__APICBase, APIC_IRR+0x60),
            APIC_REG(__APICBase, APIC_IRR+0x50), APIC_REG(__APICBase, APIC_IRR+0x40),
            APIC_REG(__APICBase, APIC_IRR+0x30), APIC_REG(__APICBase, APIC_IRR+0x20),
            APIC_REG(__APICBase, APIC_IRR+0x10), APIC_REG(__APICBase, APIC_IRR+0x00));
        bug("ISR.%03x: %08x%08x%08x%08x%08x%08x%08x%08x\n", cpunum,
            APIC_REG(__APICBase, APIC_ISR+0x70), APIC_REG(__APICBase, APIC_ISR + 0x60),
            APIC_REG(__APICBase, APIC_ISR + 0x50), APIC_REG(__APICBase, APIC_ISR + 0x40),
            APIC_REG(__APICBase, APIC_ISR + 0x30), APIC_REG(__APICBase, APIC_ISR + 0x20),
            APIC_REG(__APICBase, APIC_ISR + 0x10), APIC_REG(__APICBase, APIC_ISR + 0x00));
    )
#endif
    // An IRQ which arrived at the CPU is *either* an exception (let it be syscall, cpu exception,
    // LAPIC local irq) or a device IRQ.
    if (IS_EXCEPTION(int_number))
    {
        unsigned long exception_number = GET_EXCEPTION_NUMBER(int_number);

        DTRAP(bug("[Kernel] %s: CPU Exception %08x\n", __func__, int_number);)
        DTRAP(bug("[Kernel] %s: --> CPU Trap #$%08x\n", __func__, exception_number);)

        cpu_Trap(regs, error_code, exception_number);

        // NOT a CPU exception, must be APIC or Syscall. If APIC, send EOI
        if (exception_number >= X86_CPU_EXCEPT_COUNT && exception_number != APIC_EXCEPT_SYSCALL)
        {
            DTRAP(bug("[Kernel] %s: Sending EOI to LAPIC on CPU%03x\n", __func__, KrnGetCPUNumber());)
            IPTR __APICBase = core_APIC_GetBase();
            APIC_REG(__APICBase, APIC_EOI) = 0;
        }
    }
    else
    {
        UBYTE irq_number = GET_DEVICE_IRQ(int_number);

        DIRQ(bug("[Kernel] %s: Device IRQ #$%02X\n", __func__, irq_number);)

        if (KernelBase)
        {
            struct IntrController *irqIC;
            struct KernelInt *irqInt;

            irqInt = &KernelBase->kb_Interrupts[irq_number];

            if ((irqIC = krnGetInterruptController(KernelBase, irqInt->ki_List.lh_Type)) != NULL)
            {
                if (irqIC->ic_IntrAck)
                    irqIC->ic_IntrAck(irqIC->ic_Private, irqInt->ki_List.l_pad, irq_number);

                if (irqInt->ki_Priv & IRQINTF_ENABLED)
                {
                    if (!IsListEmpty(&irqInt->ki_List))
                        krnRunIRQHandlers(KernelBase, irq_number);

                    if ((irqIC->ic_Flags & ICF_ACKENABLE) &&
                        (irqIC->ic_IntrEnable))
                        irqIC->ic_IntrEnable(irqIC->ic_Private, irqInt->ki_List.l_pad, irq_number);
                }
            }
        }

        /*
         * Upon exit from the lowest-level device IRQ, if we are returning to user mode,
         * we check if we need to call software interrupts or run the task scheduler.
         */
        if ((SysBase) && (INTR_USERMODESTACK))
        {
            /* Disable interrupts for a while */
            __asm__ __volatile__("cli; cld;");

            core_ExitInterrupt(regs);
        }
    }

    core_LeaveInterrupt(regs);
}
