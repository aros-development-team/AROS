/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.
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

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0

 #if (DEBUG > 0)
#define D(x)
#define DIDT(x) x
#define DIRQ(x) x
#define DTRAP(x) x
#else
#define D(x)
#define DIDT(x)
#define DIRQ(x)
#define DTRAP(x)
#endif

#define DUMP_CONTEXT

//#define INTRASCII_DEBUG

#if (DEBUG > 0) && defined(INTRASCII_DEBUG)
#define DEBUGCOLOR_SET       "\033[41m"
#define DEBUGFUNCCOLOR_SET   "\033[41;1m"
#define DEBUGCOLOR_RESET     "\033[0m"
#else
#define DEBUGCOLOR_SET
#define DEBUGFUNCCOLOR_SET
#define DEBUGCOLOR_RESET
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
BOOL core_SetIDTGate(x86vectgate_t *IGATES, int vect, uintptr_t gate, BOOL enable, BOOL force)
{
    DIDT(
        APTR gateOld;

        bug("[Kernel]" DEBUGFUNCCOLOR_SET " %s: Setting IDTGate #%d IDT @ 0x%p" DEBUGCOLOR_RESET "\n", __func__, vect, IGATES);
        bug("[Kernel]" DEBUGCOLOR_SET " %s: gate @ 0x%p" DEBUGCOLOR_RESET "\n", __func__, gate);
        bug("[Kernel]" DEBUGCOLOR_SET " %s: enable=%d" DEBUGCOLOR_RESET "\n", __func__, enable);
    )
#if (__WORDSIZE != 64)
    DIDT(gateOld = (APTR)((((IPTR)IGATES[vect].offset_high & 0xFFFF) << 16) | ((IPTR)IGATES[vect].offset_low & 0xFFFF));)
#else
    DIDT(gateOld = (APTR)((((IPTR)IGATES[vect].offset_high & 0xFFFFFFFF) << 32) | (((IPTR)IGATES[vect].offset_mid & 0xFFFF) << 16) | ((IPTR)IGATES[vect].offset_low & 0xFFFF));)
#endif
    DIDT(
        if (gateOld) bug("[Kernel]" DEBUGCOLOR_SET " %s: existing gate @ 0x%p" DEBUGCOLOR_RESET "\n", __func__, gateOld);
    )

    if (IGATES[vect].p && force)
        IGATES[vect].p = 0;

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
        bug("[Kernel]" DEBUGCOLOR_SET " %s: 0x%p Vector #%d gate already enabled!" DEBUGCOLOR_RESET "\n", __func__, IGATES, vect);
    }
    return FALSE;
}

/* Set a hardware IRQ's gate in the IDT */
BOOL core_SetIRQGate(void *idt, int IRQ, uintptr_t gate)
{
    x86vectgate_t *IGATES = (x86vectgate_t *)idt;
    DIDT(
        bug("[Kernel]" DEBUGFUNCCOLOR_SET " %s: Setting IRQGate #%d" DEBUGCOLOR_RESET "\n", __func__, IRQ);
        bug("[Kernel]" DEBUGCOLOR_SET " %s: gate @ 0x%p" DEBUGCOLOR_RESET "\n", __func__, gate);
    )

    return core_SetIDTGate(IGATES, HW_IRQ_BASE + IRQ, gate, TRUE, FALSE);
}

/* Initialize the Exception gates in the IDT */
void core_SetExGates(void *idt)
{
    x86vectgate_t *IGATES = (x86vectgate_t *)idt;
    int i;

    /* Obtain/set the critical IRQs and Vectors */
    for (i = 0; i < X86_CPU_EXCEPT_COUNT; i++)
    {
        if (!core_SetIDTGate(IGATES, i, (uintptr_t)IntrDefaultGates[i], TRUE, FALSE))
        {
            krnPanic(NULL, "Failed to set CPU Exception Vector\n"
                           "Vector #$%02X\n", i);
        }
    }
}

void core_ReloadIDT()
{
    struct KernelBase *KernelBase = getKernelBase();
    struct APICData *apicData  = KernelBase->kb_PlatformData->kb_APIC;
    apicid_t cpuNo = KrnGetCPUNumber();
    struct segment_selector IDT_sel;

    x86vectgate_t *IGATES = (x86vectgate_t *)apicData->cores[cpuNo].cpu_IDT;

    DIRQ(bug("[Kernel]" DEBUGFUNCCOLOR_SET " %s()" DEBUGCOLOR_RESET "\n", __func__);)

    IDT_sel.size = sizeof(x86vectgate_t) * 256 - 1;
    IDT_sel.base = (unsigned long)IGATES;
    DIDT(bug("[Kernel]" DEBUGCOLOR_SET " %s(%u):    base 0x%p, size %d" DEBUGCOLOR_RESET "\n", __func__, cpuNo, IDT_sel.base, IDT_sel.size));

    asm volatile ("lidt %0"::"m"(IDT_sel));
}

void core_SetupIDT(apicid_t _APICID, x86vectgate_t *IGATES)
{
    int i;
    uintptr_t off;
    struct segment_selector IDT_sel;

    // TODO: ASSERT IGATES is aligned
    
    if (IGATES)
    {
        DIDT(
            bug("[Kernel]" DEBUGCOLOR_SET " %s(%u): IDT @ 0x%p" DEBUGCOLOR_RESET "\n", __func__, _APICID, IGATES);
            bug("[Kernel]" DEBUGCOLOR_SET " %s(%u): Setting default gates" DEBUGCOLOR_RESET "\n", __func__, _APICID);
        )

        // Disable ALL the default gates until something takes ownership
        for (i=0; i < 256; i++)
        {
            off = (uintptr_t)DEF_IRQRETFUNC;

            if (!core_SetIDTGate(IGATES, i, off, FALSE, TRUE))
            {
                bug("[Kernel]" DEBUGCOLOR_SET " %s(%u): gate #%d failed" DEBUGCOLOR_RESET "\n", __func__, _APICID, i);
            }
        }

        DIDT(bug("[Kernel]" DEBUGCOLOR_SET " %s(%u): Registering IDT .." DEBUGCOLOR_RESET "\n", __func__, _APICID));

        IDT_sel.size = sizeof(x86vectgate_t) * 256 - 1;
        IDT_sel.base = (unsigned long)IGATES;
        DIDT(bug("[Kernel]" DEBUGCOLOR_SET " %s(%u):    base 0x%p, size %d" DEBUGCOLOR_RESET "\n", __func__, _APICID, IDT_sel.base, IDT_sel.size));

        asm volatile ("lidt %0"::"m"(IDT_sel));
    }
    else
    {
        krnPanic(NULL, "Invalid IDT\n");
    }
    DIDT(bug("[Kernel]" DEBUGCOLOR_SET " %s(%u): IDT configured" DEBUGCOLOR_RESET "\n", __func__, _APICID));
}

void core_InvalidateIDT()
{
    struct segment_selector IDT_sel;
    DIDT(bug("[Kernel]" DEBUGFUNCCOLOR_SET " %s()" DEBUGCOLOR_RESET "\n", __func__));
    IDT_sel.size = 0;
    IDT_sel.base = 0;
    asm volatile ("lidt %0"::"m"(IDT_sel));
}

/* CPU exceptions are processed here */
void core_IRQHandle(struct ExceptionContext *regs, unsigned long error_code, unsigned long int_number)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct PlatformData *pdata = (struct PlatformData *)KernelBase->kb_PlatformData;

    // An IRQ which arrived at the CPU is *either* an exception (let it be syscall, cpu exception,
    // LAPIC local irq) or a device IRQ.
    if (IS_EXCEPTION(int_number))
    {
        unsigned long exception_number = GET_EXCEPTION_NUMBER(int_number);

        DTRAP(
            bug("[Kernel]" DEBUGCOLOR_SET " %s(%u): CPU Exception %08X" DEBUGCOLOR_RESET "\n", __func__, int_number, int_number);
            bug("[Kernel]" DEBUGCOLOR_SET " %s(%u): --> CPU Trap #$%08X" DEBUGCOLOR_RESET "\n", __func__, int_number, exception_number);
        )

        /* Store the error code for later retrieval */
        if (pdata)
        {
            switch (int_number)
            {
            /*
                 * only store the error code if the exception
                 * generates one
                 */
                case 8:
                case 10:
                case 11:
                case 12:
                case 13:
                case 14:
                case 17:
                case 21:
                case 29:
                case 30:
                    bug("[Kernel]" DEBUGCOLOR_SET " %s(%u): Exception error code %08X" DEBUGCOLOR_RESET "\n", __func__, int_number, error_code);
                    pdata->kb_LastException = int_number;
                    pdata->kb_LastExceptionError = error_code;
                    break;
            }
        }

        cpu_Trap(regs, error_code, exception_number);

        DTRAP(
            bug("[Kernel]" DEBUGCOLOR_SET " %s(%u): CPU Trap returned" DEBUGCOLOR_RESET "\n", __func__, int_number);
        )
    }
    else
    {
        UBYTE irq_number = GET_DEVICE_IRQ(int_number);

        DIRQ(
            bug("[Kernel]" DEBUGCOLOR_SET " %s(%u): Device IRQ #$%02X" DEBUGCOLOR_RESET "\n", __func__, int_number, irq_number);
        )

        if (pdata)
        {
            pdata->kb_PDFlags |= PLATFORMF_INIRQ;
#if defined(KERNEL_IRQSTORESSE)
            if (pdata->kb_FXCtx)
            {
                DIRQ(bug("[kernel]" DEBUGCOLOR_SET " %s(%d): saving to kb_FXCt @ 0x%p" DEBUGCOLOR_RESET "\n", __func__, int_number, pdata->kb_FXCtx);)
                if (KernelBase->kb_ContextSize > CPUSSEContxtSize)
                {
                    DIRQ(bug("[kernel]" DEBUGCOLOR_SET " %s(%d): AVX save" DEBUGCOLOR_RESET "\n", __func__, int_number);)
                    asm volatile("xsave (%0)"::"r"(pdata->kb_FXCtx));
                }
                else
                {
                    DIRQ(bug("[kernel]" DEBUGCOLOR_SET " %s(%d): SSE save" DEBUGCOLOR_RESET "\n", __func__, int_number);)
                    asm volatile("fxsave (%0)"::"r"(pdata->kb_FXCtx));
                }
            }
#endif
        }
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
        if (pdata)
        {
#if defined(KERNEL_IRQSTORESSE)
            if (pdata->kb_FXCtx)
            {
                DIRQ(bug("[kernel]" DEBUGCOLOR_SET " %s(%d): Device IRQ - restoring fp state from kb_FXCt @ 0x%p" DEBUGCOLOR_RESET "\n", __func__, int_number, pdata->kb_FXCtx);)
                if (KernelBase->kb_ContextSize > CPUSSEContxtSize)
                {
                    DIRQ(bug("[kernel]" DEBUGCOLOR_SET " %s(%d): AVX restore" DEBUGCOLOR_RESET "\n", __func__, int_number);)
                    asm volatile("xrstor (%0)"::"r"(pdata->kb_FXCtx));
                }
                else
                {
                    DIRQ(bug("[kernel]" DEBUGCOLOR_SET " %s(%d): SSE restore" DEBUGCOLOR_RESET "\n", __func__, int_number);)
                    asm volatile("fxrstor (%0)"::"r"(pdata->kb_FXCtx));
                }
            }
#endif
            pdata->kb_PDFlags &= ~PLATFORMF_INIRQ;
        }
        /*
         * Upon exit from the lowest-level device IRQ, if we are returning to user mode,
         * we check if we need to call software interrupts or run the task scheduler.
         */
        if (SysBase != NULL && INTR_FROMUSERMODE)
        {

            /* Disable interrupts for a while */
            __asm__ __volatile__("cli; cld;");

            DIRQ(
                bug("[Kernel]" DEBUGCOLOR_SET " %s(%u): calling ExitInterrupt... (>usermode)(%08X)" DEBUGCOLOR_RESET "\n", __func__, int_number, regs->Flags);
            )
            core_ExitInterrupt(regs);
        }
    }

    DIRQ(
        bug("[Kernel]" DEBUGCOLOR_SET " %s(%u): calling LeaveInterrupt...(%08X)" DEBUGCOLOR_RESET "\n", __func__, int_number, regs->Flags);
    )
    core_LeaveInterrupt(regs);
}
