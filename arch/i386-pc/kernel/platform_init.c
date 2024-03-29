/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.
*/

#define __KERNEL_NOLIBBASE__

#include <aros/symbolsets.h>
#include <asm/cpu.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_intr.h"

#include "utils.h"

#define D(x)
#define DSYSCALL(x)

#define TSS_SELECTOR 0x30

extern struct syscallx86_Handler x86_SCSupervisorHandler;

int core_SysCallHandler(struct ExceptionContext *regs, struct KernelBase *KernelBase, void *HandlerData2);
extern int core_APICSpuriousHandle(struct ExceptionContext *, void *, void *);

static int PlatformInit(struct KernelBase *KernelBase)
{
    struct PlatformData *data;
    struct tss      *tss = __KernBootPrivate->TSS;
    x86vectgate_t *idt = __KernBootPrivate->BOOTIDT;
    struct segment_desc *GDT = __KernBootPrivate->BOOTGDT;
    int i;

    NEWLIST(&KernelBase->kb_ICList);
    NEWLIST(&KernelBase->kb_InterruptMappings);
    KernelBase->kb_ICTypeBase = KBL_INTERNAL + 1;

    for (i = 0; i < HW_IRQ_COUNT; i++)
    {
        KernelBase->kb_Interrupts[i].ki_Priv &= ~IRQINTF_ENABLED;
        KernelBase->kb_Interrupts[i].ki_List.lh_Type = KBL_INTERNAL;
    }

    data = AllocMem(sizeof(struct PlatformData), MEMF_PUBLIC|MEMF_CLEAR);
    if (!data)
        return FALSE;
        
    D(bug("[Kernel:i386] %s: Allocated platform data at 0x%p\n", __func__, data));
    KernelBase->kb_PlatformData = data;

    /*
     * Now we have a complete memory list and working AllocMem().
     * We can build IDT and TSS now to make interrupts work.
     */
    SysBase->SysStkLower = AllocMem(0x10000, MEMF_PUBLIC);  /* 64KB of system stack */

    if (!SysBase->SysStkLower)
        return FALSE;

    tss->ssp_seg = KERNEL_DS; /* SSP segment descriptor */
    tss->cs      = USER_CS;
    tss->ds      = USER_DS;
    tss->es      = USER_DS;
    tss->ss      = USER_DS;
    tss->iomap   = 104;

    /* Set up system stack */
    SysBase->SysStkUpper = SysBase->SysStkLower + 0x10000;
    tss->ssp       = (IPTR)SysBase->SysStkUpper;

    /* Restore IDT structure */
    core_SetupIDT(0, idt);

    // Set up the base syscall handler(s)
    NEWLIST(&data->kb_SysCallHandlers);
    /*
     * Setup the BSP's exception, spurious, heartbeat and syscall gates early
    */
    core_SetExGates((x86vectgate_t *)idt);
    if (!core_SetIDTGate((x86vectgate_t *)idt, APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_SYSCALL),
                                    (uintptr_t)IntrDefaultGates[APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_SYSCALL)], TRUE, FALSE))
    {
        krnPanic(NULL, "Failed to set BSP Syscall Vector\n"
                       "Vector #%02X\n",
                 APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_SYSCALL));
    }
    if (!core_SetIDTGate((x86vectgate_t *)idt, APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_HEARTBEAT),
                                    (uintptr_t)IntrDefaultGates[APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_HEARTBEAT)], TRUE, FALSE))
    {
        krnPanic(NULL, "Failed to set BSP Heartbeat Vector\n"
                       "Vector #%02X\n",
                 APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_HEARTBEAT));
    }
    if (!core_SetIDTGate((x86vectgate_t *)idt, APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_SPURIOUS),
                                    (uintptr_t)IntrDefaultGates[APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_SPURIOUS)], TRUE, FALSE))
    {
        krnPanic(NULL, "Failed to set BSP Spurious Vector\n"
                       "Vector #%02X\n",
                 APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_SPURIOUS));
    }
    KrnAddExceptionHandler(APIC_EXCEPT_SPURIOUS, core_APICSpuriousHandle, KernelBase, NULL);
    KrnAddExceptionHandler(APIC_EXCEPT_SYSCALL, core_SysCallHandler, KernelBase, NULL);
    krnAddSysCallHandler(data, &x86_SCSupervisorHandler, FALSE, TRUE);
    D(bug("[Kernel:i386] %s: BSP vectors and handlers configured\n", __func__));

    /* Set correct TSS address in the GDT */
    GDT[6].base_low  = ((unsigned long)tss) & 0xffff;
    GDT[6].base_mid  = (((unsigned long)tss) >> 16) & 0xff;
    GDT[6].base_high = (((unsigned long)tss) >> 24) & 0xff;

    /*
     * As we prepared all necessary stuff, we can hopefully load IDT
     * into CPU. We may also play a bit with TSS
     */
    asm
    (
        "ltr %%ax\n\t"
        ::"ax"(TSS_SELECTOR)
    );

    D(bug("[Kernel:i386] %s: System restored\n", __func__));

    return TRUE;
}

ADD2INITLIB(PlatformInit, 5);
