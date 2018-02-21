/*
    Copyright © 2011-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intel 8259A "Legacy" PIC driver.
*/

#include <proto/exec.h>

#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>

#include <asm/io.h>
#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_interrupts.h"

#define D(x)
#define DINT(x)

struct i8259a_Instance
{
    UWORD       irq_mask;
    UBYTE       irq_base;
};

struct i8259a_Private
{
    struct i8259a_Instance      irq_ic[0];
};

icid_t i8259a_Register(struct KernelBase *KernelBase)
{
    DINT(bug("[Kernel:i8259a] %s()\n", __func__));

    /* if we have been disabled, fail to register */
    if (i8259a_IntrController.ic_Flags & ICF_DISABLED)
        return (icid_t)-1;

    i8259a_IntrController.ic_Flags |= ICF_ACKENABLE;

    return (icid_t)i8259a_IntrController.ic_Node.ln_Type;
}

BOOL i8259a_DisableIRQ(APTR icPrivate, icid_t icInstance, icid_t intNum)
{
    struct i8259a_Private *xtpicPriv= (struct i8259a_Private *)icPrivate;
    struct i8259a_Instance *xtPic = (struct i8259a_Instance *)&xtpicPriv->irq_ic[icInstance];

    DINT(bug("[Kernel:i8259a] %s(0x%p, %d, %d)\n", __func__, icPrivate, icInstance, intNum));

    intNum &= 15;

    if (intNum == 2)
    {
    	/* IRQ2 must never be disabled. Doing so breaks communication between two 8259s */
    	return FALSE;
    }
    xtPic->irq_mask |= 1 << intNum;

    if (intNum >= 8)
        outb((xtPic->irq_mask >> 8) & 0xff, SLAVE8259_MASKREG);
    else
        outb(xtPic->irq_mask & 0xff, MASTER8259_MASKREG);

    return TRUE;
}

BOOL i8259a_EnableIRQ(APTR icPrivate, icid_t icInstance, icid_t intNum) // uint16_t *irqmask)
{
    struct i8259a_Private *xtpicPriv= (struct i8259a_Private *)icPrivate;
    struct i8259a_Instance *xtPic = (struct i8259a_Instance *)&xtpicPriv->irq_ic[icInstance];

    DINT(bug("[Kernel:i8259a] %s(0x%p, %d, %d)\n", __func__, icPrivate, icInstance, intNum));

    intNum &= 15;

    if (intNum == 2)
    {
        /* IRQ2 is always enabled anyway, and it's not a "real" IRQ, so it's probably 
         * appropriate to report failure */
    	return FALSE;
    }
    xtPic->irq_mask &= ~(1 << intNum);    

    if (intNum >= 8)
        outb((xtPic->irq_mask >> 8) & 0xff, SLAVE8259_MASKREG);
    else
        outb(xtPic->irq_mask & 0xff, MASTER8259_MASKREG);

    return TRUE;
}

/*
 * Careful! The 8259A is a fragile beast, it pretty much _has_ to be done exactly like this (mask it
 * first, _then_ send the EOI, and the order of EOI to the two 8259s is important!
 */
BOOL i8259a_AckIntr(APTR icPrivate, icid_t icInstance, icid_t intNum) // uint16_t *irqmask)
{
    struct i8259a_Private *xtpicPriv= (struct i8259a_Private *)icPrivate;
    struct i8259a_Instance *xtPic = (struct i8259a_Instance *)&xtpicPriv->irq_ic[icInstance];

    DINT(bug("[Kernel:i8259a] %s(0x%p, %d, %d)\n", __func__, icPrivate, icInstance, intNum));

    intNum &= 15;
    xtPic->irq_mask |= 1 << intNum;

    if (intNum >= 8)
    {
        outb((xtPic->irq_mask >> 8) & 0xff, SLAVE8259_MASKREG);
        outb(0x62, MASTER8259_CMDREG);
        outb(0x20, SLAVE8259_CMDREG);
    }
    else
    {
        outb(xtPic->irq_mask & 0xff, MASTER8259_MASKREG);
        outb(0x20, MASTER8259_CMDREG);
    }

    return TRUE;
}


BOOL i8259a_Init(struct KernelBase *KernelBase, icid_t instanceCount)
{
    struct i8259a_Private *xtpicPriv;
    struct i8259a_Instance *xtPic;
    int instance, irq;

    DINT(bug("[Kernel:i8259a] %s(%d)\n", __func__, instanceCount));

    /* Sanity check */
    if (i8259a_IntrController.ic_Flags & ICF_DISABLED)
        return FALSE;

    xtpicPriv = (struct i8259a_Private *)
        AllocMem(sizeof(struct i8259a_Private)
        + sizeof(struct i8259a_Instance) * instanceCount, MEMF_ANY);

    if ((i8259a_IntrController.ic_Private = xtpicPriv) != NULL)
    {
        /*
         * After initialization everything is disabled except cascade interrupt (IRQ 2).
         * Be careful and don't miss this! Without IRQ2 enabled slave 8259 (and its IRQs)
         * will not work!
         */

        /* Setup Masks */
        for (instance = 0; instance < instanceCount; instance++)
        {
            int instIRQBase = (instance * I8259A_IRQCOUNT);

            DINT(bug("[Kernel:i8259a] %s: initializing instance #%d\n", __func__, instance));

            xtPic = (struct i8259a_Instance *)&xtpicPriv->irq_ic[instance];
            xtPic->irq_mask = 0xFFFF;
            xtPic->irq_base = HW_IRQ_BASE + instIRQBase;               /* route irqs after the cpu's exceptions */
            if (instance == 0)
            {
                APTR ssp = NULL;

                if ((KrnIsSuper()) || ((ssp = SuperState()) != NULL))
                {
                    /* Take over the first 8259A's IRQs */
                    for (irq = instIRQBase; irq < (instIRQBase + I8259A_IRQCOUNT); irq++)
                    {
                        if (!krnInitInterrupt(KernelBase, irq, i8259a_IntrController.ic_Node.ln_Type, instance))
                        {
                            D(bug("[Kernel:i8259a] %s: failed to acquire IRQ #%d\n", __func__, irq);)
                        }
                        else
                        {
                            if ((irq - instIRQBase) != 2)
                            {
                                if (!core_SetIRQGate(
                                    (struct int_gate_64bit *)__KernBootPrivate->BOOTIDT,
                                    irq, (uintptr_t)IntrDefaultGates[HW_IRQ_BASE + irq]))
                                {
                                    bug("[Kernel:i8259a] %s: failed to set IRQ %d's gate\n", __func__, irq);
                                }
                                if (ictl_is_irq_enabled(irq, KernelBase))
                                    xtPic->irq_mask &= ~(1 << (irq - instIRQBase));
                                else
                                    xtPic->irq_mask |= (1 << (irq - instIRQBase));
                            }
                            else
                                xtPic->irq_mask &= ~(1 << (irq - instIRQBase));
                        }
                    }
                    if (ssp)
                        UserState(ssp);
                }
                /* Set up the first registered 8259. Send four ICWs (see 8259 datasheet) */
                asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x11),"i"(MASTER8259_CMDREG)); /* Initialization sequence for 8259A-1 (edge-triggered, cascaded, ICW4 needed) */
                asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x11),"i"(SLAVE8259_CMDREG)); /* Initialization sequence for 8259A-2, the same as above */
                asm("outb   %b0,%b1\n\tcall delay"::"a"(xtPic->irq_base),"i"(MASTER8259_MASKREG)); /* IRQs for master */
                asm("outb   %b0,%b1\n\tcall delay"::"a"(xtPic->irq_base + 8),"i"(SLAVE8259_MASKREG)); /* IRQs for slave */
                asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x04),"i"(MASTER8259_MASKREG)); /* 8259A-1 is master, slave at IRQ2 */
                asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x02),"i"(SLAVE8259_MASKREG)); /* 8259A-2 is slave, ID = 2 */
                asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x01),"i"(MASTER8259_MASKREG)); /* 8086 mode, non-buffered, nonspecial fully nested mode for both */
                asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x01),"i"(SLAVE8259_MASKREG));

                /* Now initialize interrupt masks */
                asm("outb   %b0,%b1\n\tcall delay"::"a"((char)(xtPic->irq_mask & 0xFF)),"i"(MASTER8259_MASKREG)); /* Enable cascade int */
                asm("outb   %b0,%b1\n\tcall delay"::"a"((char)(xtPic->irq_mask >> 8)),"i"(SLAVE8259_MASKREG)); /* Mask all interrupts */
            }
        }
        DINT(bug("[Kernel:i8259a] %s: complete\n", __func__));
        return TRUE;
    }
    bug("[Kernel:i8259a] Failed to allocate controller storage!\n");
    return FALSE;
}

/* Small delay routine used by i8259a_Init() initializer */
asm("\ndelay:\t.short   0x00eb\n\tret");

struct IntrController i8259a_IntrController =
{
    {
        .ln_Name = "8259A PIC",
        .ln_Pri = 0
    },
    0,
    ICTYPE_I8259A,
    0,
    NULL,
    i8259a_Register,
    i8259a_Init,
    i8259a_EnableIRQ,
    i8259a_DisableIRQ,
    i8259a_AckIntr,
};

void i8259a_Disable()
{
    /* Mask all pic interrupts */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0xFF),"i"(MASTER8259_MASKREG));
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0xFF),"i"(SLAVE8259_MASKREG));
}

BOOL i8259a_Probe()
{
    BYTE maskres;

    D(bug("[Kernel:i8259a] %s()\n", __func__));

    /* mask all of the interrupts except the cascade pin */
    outb(0xff, SLAVE8259_MASKREG);      
    outb(~(1 << 2), MASTER8259_MASKREG);
    maskres = inb(MASTER8259_MASKREG);
    if (maskres == ~(1 << 2))
        return TRUE;
    return FALSE;
}
