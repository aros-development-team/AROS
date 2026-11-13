#ifndef KERNEL_INTERRUPTCONTROLLERS_H
#define KERNEL_INTERRUPTCONTROLLERS_H
/*
    Copyright (C) 2017-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#ifdef KERNELIRQ_NEEDSCONTROLLERS
#include <inttypes.h>

#include <kernel_irqtypes.h>

/* Interrupt controller definitions ... */

struct IntrInstance;

/*
 * Details:
 *
 * .ln_Node = "Controller Type" Name - set by the "driver".
 * .ln_Pri = use count - value filled in by/returned from krnAddInterruptController.
 *
 * The id lives in ic_Id, not in ln_Type: ln_Type is a UBYTE by the exec ABI,
 * and on a target with KRN_ICID_BITS > 8 it cannot hold the value.
*/
struct IntrController
{
    struct Node ic_Node;
    ULONG        ic_Count;
    ULONG        ic_Type;                                                     /* IC drivers private "type"                              */
    ULONG        ic_Flags;
    APTR        ic_Private;
    icid_t      ic_Id;                                                        /* assigned by/returned from krnAddInterruptController     */
    icid_t      (*ic_Register)(struct KernelBase *);                          /* one time initialization called during Add              */
    BOOL        (*ic_Init)(struct KernelBase *, icinstid_t);                  /* passed the number of instances to bring up             */
    BOOL        (*ic_IntrEnable)(APTR, icinstid_t, irqid_t);
    BOOL        (*ic_IntrDisable)(APTR, icinstid_t, irqid_t);
    BOOL        (*ic_IntrAck)(APTR, icinstid_t, irqid_t);
};

struct IntrMapping
{
    struct Node im_Node;
    irqid_t     im_DeviceIRQ;                                                   /* device IRQ as used by KrnAddIRQHandler()             */
    ULONG       im_Int;                                                         /* controller specific hardware interrupt to use        */
    ULONG       im_CPU;                                                         /* target CPU for interrupt delivery                    */
    UBYTE       im_Polarity;                                                    /* 0 = Default, 1 = HIGH, 2 = LOW                       */
    UBYTE       im_Trig;                                                        /* 0 = Default, 1 = LEVEL, 2 = EDGE                     */
};

/*
 * Interrupt controller needs to re-enable
 * the interrupt after acknowledging/processing
 */
#define ICF_ACKENABLE   (1 << 0)

#define ICF_READY       (1 << 30)
#define ICF_DISABLED    (1 << 31)

static inline struct IntrController *krnGetInterruptController(struct KernelBase *KernelBase, icid_t icid)
{
    struct IntrController *intContr;
    ForeachNode(&KernelBase->kb_ICList, intContr)
    {
        if (intContr->ic_Id == icid)
        {
            return intContr;
        }
    }
    return NULL;
}

/* Functions to be called by machine-specific code */
icintrid_t krnAddInterruptController(struct KernelBase *, struct IntrController *);
struct IntrController *krnFindInterruptController(struct KernelBase *, ULONG);
int krnInitInterruptControllers(struct KernelBase *);
BOOL krnInitInterrupt(struct KernelBase *, irqid_t, icid_t, icinstid_t);
struct IntrMapping *krnInterruptMapping(struct KernelBase *, irqid_t);
struct IntrMapping *krnInterruptMapped(struct KernelBase *, ULONG);

#endif /* KERNELIRQ_NEEDSCONTROLLERS */
#endif /* !KERNEL_INTERRUPTCONTROLLERS_H */
