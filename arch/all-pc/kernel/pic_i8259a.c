/*
    Copyright © 2011-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intel 8259A "Legacy" PIC driver.
*/

#include <proto/exec.h>

#include <asm/io.h>
#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_debug.h"
#include "kernel_interrupts.h"
#include "i8259a.h"

#define D(x)

struct i8259a_Instance
{
    ULONG       irq_mask;
    UBYTE       irq_base;
};

struct i8259a_Private
{
    struct i8259a_Instance      irq_ic[0];
};

icid_t i8259a_Register(struct KernelBase *KernelBase)
{
    D(bug("[Kernel:i8259a] %s()\n", __func__));

    /* if we have been disabled, fail to register */
    if (i8259a_IntrController.ic_Flags & ICF_DISABLED)
        return -1;

    i8259a_IntrController.ic_Flags |= ICF_ACKENABLE;

    return (icid_t)i8259a_IntrController.ic_Node.ln_Type;
}

BOOL i8259a_Init(struct KernelBase *KernelBase, icid_t instanceCount)
{
    struct i8259a_Private *xtpicPriv;
    struct i8259a_Instance *xtPic;
    int i;

    D(bug("[Kernel:i8259a] %s(%d)\n", __func__, instanceCount));

    /* sanity check .. */
    if (i8259a_IntrController.ic_Flags & ICF_DISABLED)
        return FALSE;

    xtpicPriv = (struct i8259a_Private *)AllocMem(sizeof(struct i8259a_Instance) * instanceCount, MEMF_ANY);
    if ((i8259a_IntrController.ic_Private = xtpicPriv) != NULL)
    {
        /* Take over the 8259a IRQ's */
        for (i = 0; i < I8259A_IRQCOUNT; i++)
        {
            if (!krnInitInterrupt(KernelBase, i, i8259a_IntrController.ic_Node.ln_Type, 0))
            {
                bug("[Kernel:i8259a] %s: failed to acquire IRQ #%d\n", __func__, i);
            }
        }

        /*
         * After initialization everything is disabled except cascade interrupt (IRQ 2).
         * Be careful and don't miss this! Without IRQ2 enabled slave 8259 (and its IRQs)
         * will not work!
         */

        /* Setup Masks */
        for (i = 0; i < instanceCount; i++)
        {
            xtPic = (struct i8259a_Instance *)&xtpicPriv->irq_ic[i];
            xtPic->irq_mask = 0xFFFB;
            xtPic->irq_base = 0x20;               /* route irqs after the cpu's exceptions */
        }

        /* Setup the first registered 8259. Send four ICWs (see 8529 datasheet) */
        xtPic = (struct i8259a_Instance *)&xtpicPriv->irq_ic[0];
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x11),"i"(MASTER8259_CMDREG)); /* Initialization sequence for 8259A-1 (edge-triggered, cascaded, ICW4 needed) */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x11),"i"(SLAVE8259_CMDREG)); /* Initialization sequence for 8259A-2, the same as above */
        asm("outb   %b0,%b1\n\tcall delay"::"a"(xtPic->irq_base),"i"(MASTER8259_MASKREG)); /* IRQs for master */
        asm("outb   %b0,%b1\n\tcall delay"::"a"(xtPic->irq_base + 8),"i"(SLAVE8259_MASKREG)); /* IRQs for slave */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x04),"i"(MASTER8259_MASKREG)); /* 8259A-1 is master, slave at IRQ2 */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x02),"i"(SLAVE8259_MASKREG)); /* 8259A-2 is slave, ID = 2 */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x01),"i"(MASTER8259_MASKREG)); /* 8086 mode, non-buffered, nonspecial fully nested mode for both */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x01),"i"(SLAVE8259_MASKREG));

        /* Now initialize interrupt masks */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0xfb),"i"(MASTER8259_MASKREG)); /* Enable cascade int */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0xff),"i"(SLAVE8259_MASKREG)); /* Mask all interrupts */

        return TRUE;
    }
    return FALSE;
}

/* Small delay routine used by i8259a_Init() initializer */
asm("\ndelay:\t.short   0x00eb\n\tret");

BOOL i8259a_DisableIRQ(APTR icPrivate, icid_t icInstance, icid_t intNum)
{
    struct i8259a_Private *xtpicPriv= (struct i8259a_Private *)icPrivate;
    struct i8259a_Instance *xtPic = (struct i8259a_Instance *)&xtpicPriv->irq_ic[icInstance];

    D(bug("[Kernel:i8259a] %s(0x%p, %d, %d)\n", __func__, icPrivate, icInstance, intNum));

    intNum &= 15;

    if (intNum == 2)
    {
    	/* IRQ2 must never be disabled. Doing so breaks communication between two 8259's */
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

    D(bug("[Kernel:i8259a] %s(0x%p, %d, %d)\n", __func__, icPrivate, icInstance, intNum));

    intNum &= 15;
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

    D(bug("[Kernel:i8259a] %s(0x%p, %d, %d)\n", __func__, icPrivate, icInstance, intNum));

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

struct IntrController i8259a_IntrController =
{
    {
        .ln_Name = "8259A PIC"
    },
    ICTYPE_I8259A,
    0,
    NULL,
    i8259a_Register,
    i8259a_Init,
    i8259a_EnableIRQ,
    i8259a_DisableIRQ,
    i8259a_AckIntr,
};

BOOL i8259a_Probe()
{
    UBYTE maskres;

    D(bug("[Kernel:i8259a] %s()\n", __func__));

    /* mask all of the interrupts except the cascade pin */
    outb(0xff, SLAVE8259_MASKREG);      
    outb(~(1 << 2), MASTER8259_MASKREG);
    maskres = inb(MASTER8259_MASKREG);
    if (maskres == ~(1 << 2))
        return TRUE;
    return FALSE;
}
