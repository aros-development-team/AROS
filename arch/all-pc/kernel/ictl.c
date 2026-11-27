/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/arossupport.h>

#include <string.h>

#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_interrupts.h"
#include "kernel_debug.h"

#include "apic.h"
#include "pic_i8259a.h"

#define D(x)

/*
 * One lock for what the interrupt controllers keep for all CPUs: the IOAPIC
 * register window and routing tables, the 8259A masks, the present bits of
 * the device vector gates in every CPU's IDT, the vector and MSI allocators
 * and the interrupt mappings list. The controller code takes it itself.
 *
 * It is not kb_IntrSpinLock, and cannot be. That one is held for writing
 * around ic_IntrDisable by KrnRemIRQHandler(), and for reading only at
 * interrupt time, while two CPUs acknowledging through one IOAPIC still
 * have to exclude each other.
 *
 * Interrupts are off while it is held, as the acknowledge path takes it.
 * A task gets that from Disable(), not from a bare cli: KrnSpinLock() may
 * call SetTaskPri() while the task waits, and the Enable() in there would
 * turn interrupts back on. In supervisor mode Enable() never turns them
 * on, so there the flags are saved and restored instead. The mode must be
 * the same at both ends.
 *
 * A syscall is answered through krnRunExceptionHandlers(), which reads
 * under kb_IntrSpinLock. KrnRemIRQHandler() waits for this lock with that
 * one held for writing, so a holder of this lock that made a syscall would
 * wait for it in turn. Code that needs supervisor mode enters it first.
 */
#if defined(__AROSEXEC_SMP__)
static spinlock_t ictl_SpinLock;

IPTR ictl_Lock(void)
{
    unsigned long flags = 0;

    if (KrnIsSuper())
    {
        /* Never zero: bit 1 of the flags register is always set */
        __save_flags(flags);
        __cli();
    }
    else
        Disable();

    KrnSpinLock(&ictl_SpinLock, NULL, SPINLOCK_MODE_WRITE);

    return (IPTR)flags;
}

void ictl_Unlock(IPTR state)
{
    unsigned long flags = (unsigned long)state;

    KrnSpinUnLock(&ictl_SpinLock);

    if (flags)
        __restore_flags(flags);
    else
        Enable();
}
#else
/* One CPU: the callers' own Disable() and the interrupt gate are enough */
IPTR ictl_Lock(void)
{
    return 0;
}

void ictl_Unlock(IPTR state)
{
}
#endif

void ictl_enable_irq(irqid_t irq, struct KernelBase *KernelBase)
{
    struct IntrController *irqIC;
    struct KernelInt *irqInt;

    if (irq >= HW_IRQ_COUNT)
        return;

    irqInt = &KernelBase->kb_Interrupts[irq];

    D(bug("[Kernel] %s(%d)\n", __func__, irq));

    if ((irqIC = krnGetInterruptController(KernelBase, irqInt->ki_ICId)) != NULL)
    {
        if ((irqIC->ic_IntrEnable) && (irqIC->ic_IntrEnable(irqIC->ic_Private, irqInt->ki_ICInst, irq)))
        {
            D(bug("[Kernel] %s: controller enabled\n", __func__));
            irqInt->ki_Priv |= IRQINTF_ENABLED;
        }
    }
    else
    {
        D(bug("[Kernel] %s: enabled\n", __func__));
        irqInt->ki_Priv |= IRQINTF_ENABLED;
    }
}

void ictl_disable_irq(irqid_t irq, struct KernelBase *KernelBase)
{
    struct IntrController *irqIC;
    struct KernelInt *irqInt;

    if (irq >= HW_IRQ_COUNT)
        return;

    irqInt = &KernelBase->kb_Interrupts[irq];

    D(bug("[Kernel] %s(%d)\n", __func__, irq));

    if ((irqIC = krnGetInterruptController(KernelBase, irqInt->ki_ICId)) != NULL)
    {
        if ((irqIC->ic_IntrDisable) && (irqIC->ic_IntrDisable(irqIC->ic_Private, irqInt->ki_ICInst, irq)))
        {
            D(bug("[Kernel] %s: controller disabled\n", __func__));
            irqInt->ki_Priv &= ~IRQINTF_ENABLED;
        }
    }
    else
    {
        D(bug("[Kernel] %s: disabled\n", __func__));
        irqInt->ki_Priv &= ~IRQINTF_ENABLED;
    }
}

BOOL ictl_is_irq_enabled(irqid_t irq, struct KernelBase *KernelBase)
{
    struct KernelInt *irqInt;

    if (irq >= HW_IRQ_COUNT)
        return FALSE;

    irqInt = &KernelBase->kb_Interrupts[irq];

    D(bug("[Kernel] %s(%d)\n", __func__, irq));

    return (BOOL)(irqInt->ki_Priv & IRQINTF_ENABLED);
}

void ictl_Initialize(struct KernelBase *KernelBase)
{
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    int cnt;
    struct TagItem *cmdTags;

    D(bug("[Kernel] %s()\n", __func__));

    if (!pdata->kb_APIC)
    {
        /* No APIC was discovered by ACPI/whatever else. Do the probe. */
        cmdTags = LibFindTagItem(KRN_CmdLine, BootMsg);
        if (cmdTags && strstr((const char *)cmdTags->ti_Data, "noapic"))
            D(bug("[Kernel] %s: APIC support disabled\n", __func__));
        else
        {
            D(bug("[Kernel] %s(): probing for APIC\n", __func__));
            pdata->kb_APIC = core_APIC_Probe();
        }
    }

#if (__WORDSIZE==64)
    if (!pdata->kb_APIC)
    {
        /* We are x86-64, so we should always have APIC. */
        krnPanic(KernelBase, "Failed to allocate APIC descriptor.");
    }
#endif

    /* Check if the 8259A has already been registered, if not probe for it */
    if (!krnFindInterruptController(KernelBase, IIC_ID_I8259A))
    {
        D(__unused icintrid_t xtpicICInstID;)

        D(bug("[Kernel] %s(): probing for PIC\n", __func__));
        if (i8259a_Probe())
        {
            D(xtpicICInstID =) krnAddInterruptController(KernelBase, &i8259a_IntrController);
        }
    }

    if ((cnt = krnInitInterruptControllers(KernelBase)) > 0)
    {
        D(bug("[Kernel] %s: %d Interrupt Controllers Initialized\n", __func__, cnt));
        return;
    }

    krnPanic(KernelBase, "Failed to detect any Interrupt Controllers.");
}
