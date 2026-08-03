/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
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
#include "segments.h"
#include "debug_xmm.h"

/* struct ExceptionContext member names differ between i386 and x86_64 */
#if defined(__x86_64__)
#define EXCTX_IP(regs) ((regs)->rip)
#define EXCTX_SP(regs) ((regs)->rsp)
#else
#define EXCTX_IP(regs) ((regs)->eip)
#define EXCTX_SP(regs) ((regs)->esp)
#endif

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

/*****************************************************************************
 * Vector ↔ IRQ mapping and allocation
 *
 * The IDT has 256 entries but the software IRQ array (kb_Interrupts[])
 * can be larger (HW_IRQ_COUNT=65536) to accommodate wide software IRQ spaces.
 * This mapping decouples IDT vectors from software IRQ numbers.
 *****************************************************************************/

ULONG apicIRQVectorMap[APIC_IRQ_MAX];   /* vector → software IRQ (0xFFFFFFFF = unmapped) */

static ULONG vectorBitmap[8];           /* 256 bits — tracks allocated vectors */
#define VECTOR_SET(v)    (vectorBitmap[(v) >> 5] |= (1U << ((v) & 31)))
#define VECTOR_CLR(v)    (vectorBitmap[(v) >> 5] &= ~(1U << ((v) & 31)))
#define VECTOR_ISSET(v)  (vectorBitmap[(v) >> 5] & (1U << ((v) & 31)))

void apicInitVectorMap(void)
{
    int i;

    for (i = 0; i < 8; i++)
        vectorBitmap[i] = 0;
    for (i = 0; i < APIC_IRQ_MAX; i++)
        apicIRQVectorMap[i] = 0xFFFFFFFF;

    /* Reserve CPU exception vectors 0-31 */
    for (i = 0; i < X86_CPU_EXCEPT_COUNT; i++)
        VECTOR_SET(i);

    /* Reserve APIC exception vectors 246-255 */
    for (i = APIC_CPU_EXCEPT_BASE; i < APIC_IRQ_MAX; i++)
        VECTOR_SET(i);

    /* Legacy PIC vectors 32-47 → IRQ 0-15 (always 1:1) */
    for (i = 0; i < I8259A_IRQCOUNT; i++)
    {
        VECTOR_SET(HW_IRQ_BASE + i);
        apicIRQVectorMap[HW_IRQ_BASE + i] = i;
    }
}

void apicRegisterVector(UBYTE vector, ULONG irq)
{
    VECTOR_SET(vector);
    apicIRQVectorMap[vector] = irq;
}

UBYTE apicAllocVector(ULONG irq)
{
    int i;

    for (i = APIC_IRQ_BASE; i < APIC_CPU_EXCEPT_BASE; i++)
    {
        if (!VECTOR_ISSET(i))
        {
            VECTOR_SET(i);
            apicIRQVectorMap[i] = irq;
            return (UBYTE)i;
        }
    }
    return 0;   /* pool exhausted */
}

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
    /*
     * Make sure the vector->IRQ map is usable even on systems that never
     * bring up the APIC controller (legacy PIC-only boots) - otherwise
     * core_IRQHandle would read an all-zero map and deliver every device
     * interrupt as IRQ 0. First caller wins; APICInt_Init re-initialises
     * the map explicitly when it takes ownership.
     */
    static BOOL vecmap_ready = FALSE;
    if (!vecmap_ready)
    {
        apicInitVectorMap();
        vecmap_ready = TRUE;
    }

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

        /*
         * Install the correct ISR handler for every vector but leave all gates
         * disabled (p=0).  This ensures AP IDTs have valid handlers from the
         * start — when EnableIRQ later sets p=1 on all CPUs the gate is ready.
         */
        for (i=0; i < 256; i++)
        {
            off = (uintptr_t)IntrDefaultGates[i];

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


#if DEBUG_XMM
DEFINEPSEUDOSTACK
#endif

/*
    Naming convention:
    struct ExceptionContext *regs - always represents an area on a valid stack
    struct ExceptionContext *ctx - always represents et_RegFrame which is not a valid stack
*/

/* CPU exceptions are processed here */
void core_IRQHandle(struct ExceptionContext *regs, unsigned long error_code, unsigned long int_number)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct PlatformData *pdata = NULL;

#if DEBUG_XMM
CREATEPSEUDOSTACK
#endif

#if (__WORDSIZE==64)
    /* Preserve first four XMM registers */
    asm volatile (
        "       movaps %%xmm0, (%0)\n"
        "       movaps %%xmm1, 16(%0)\n"
        "       movaps %%xmm2, 32(%0)\n"
        "       movaps %%xmm3, 48(%0)\n"
        ::"r"(regs->FXSData));
#endif

#if DEBUG_XMM
PSEUDOSTACK_MAKEFRAME
SETLOCALAREA
SAVE_XMM_INTO_AREA(localarea)
#endif

    if (KernelBase && (pdata = (struct PlatformData *)KernelBase->kb_PlatformData) != NULL)
    {
        /* cache the current state */
        pdata->kb_LastState = ((pdata->kb_PDFlags & (PLATFORMF_INIRQ|PLATFORMF_INEXCPT)) << 16) | pdata->kb_LastInt;
        pdata->kb_LastInt = int_number;
    }

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
            pdata->kb_PDFlags |= PLATFORMF_INEXCPT;
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
                    if (int_number == 14)
                    {
                        IPTR cr2 = 0;
                        __asm__ volatile("mov %%cr2,%0" : "=r"(cr2));
                        bug("[PF-DBG] CR2=%p IP=%p SP=%p CS=%p SS=%p ERR=%08lx\n",
                            (APTR)cr2, (APTR)EXCTX_IP(regs), (APTR)EXCTX_SP(regs),
                            (APTR)regs->cs, (APTR)regs->ss, error_code);
                        bug("[PF-DBG] access=%s mode=%s present=%s rsvd=%s ifetch=%s\n",
                            (error_code & 2) ? "write" : "read",
                            (error_code & 4) ? "user" : "supervisor",
                            (error_code & 1) ? "protection" : "not-present",
                            (error_code & 8) ? "yes" : "no",
                            (error_code & 16) ? "yes" : "no");
                        /*
                         * Dump the top of the faulting stack. For a call
                         * through a NULL/garbage pointer, [RSP] holds the
                         * caller's return address - names the culprit
                         * without needing the alert requester's Log button.
                         */
                        if ((error_code & 4) && EXCTX_SP(regs) >= 0x1000)
                        {
                            IPTR *sp = (IPTR *)EXCTX_SP(regs);
                            int di;
                            for (di = 0; di < 8; di++)
                            {
                                bug("[PF-STK] +%02x %p\n", di * (int)sizeof(IPTR),
                                    (APTR)sp[di]);
                            }
                        }
                    }
                    break;
            }
        }

        cpu_Trap(regs, error_code, exception_number);

        /*
         * Disable interrupts after cpu_Trap returns. The ACPI PM idle handler
         * executes sti;hlt inside cpu_Trap, and SoftIntDispatch (via core_Cause)
         * calls KrnSti(), leaving interrupts ENABLED on return. Without this cli,
         * a nested timer interrupt can corrupt the kernel-mode IRETQ frame on the
         * exception return path, causing #GP at garbage RIP (e.g. SMP trampoline).
         */
        __asm__ __volatile__("cli");

        DTRAP(
            bug("[Kernel]" DEBUGCOLOR_SET " %s(%u): CPU Trap returned" DEBUGCOLOR_RESET "\n", __func__, int_number);
        )
        if (pdata)
            pdata->kb_PDFlags &= ~PLATFORMF_INEXCPT;
    }
    else
    {
        ULONG irq_number = apicIRQVectorMap[int_number];

        DIRQ(
            bug("[Kernel]" DEBUGCOLOR_SET " %s(%u): Device IRQ #$%02X" DEBUGCOLOR_RESET "\n", __func__, int_number, irq_number);
        )

        if (pdata)
        {
            pdata->kb_PDFlags |= PLATFORMF_INIRQ;
        }
        if (KernelBase && irq_number != 0xFFFFFFFF && irq_number < HW_IRQ_COUNT)
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

#if (__WORDSIZE==64)
    /* Restore first four XMM registers. They could have been modified by any interrupt handler.
       Interrupt handler or soft interrupt code is required to preserve XMM registers 5-15. */
    /* If we are here, we are either exiting from a nested interrupt or we are exiting to user mode
       but without task switch. If task switch happened, we had already exited in cpu_Dispatch via
       core_LeaveInterrupt with first restoring all XMM/YMM registers from cpu context. If this
       was a SC_SUPERVISOR call, we had already exited via core_Supervisor. In such case registers were
       not restored, but whole flow was inside of kernel which is guaranteed not to use XMM/YMM */
    asm volatile (
        "       movaps (%0), %%xmm0\n"
        "       movaps 16(%0), %%xmm1\n"
        "       movaps 32(%0), %%xmm2\n"
        "       movaps 48(%0), %%xmm3\n"
        ::"r"(regs->FXSData));
#endif

#if DEBUG_XMM
SAVE_XMM_AND_CHECK
PSEUDOSTACK_POPFRAME
#endif

    core_LeaveInterrupt(regs);
}
