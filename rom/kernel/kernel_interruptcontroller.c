/*
    Copyright (C) 2017-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <kernel_base.h>

#ifdef KERNELIRQ_NEEDSCONTROLLERS

#include <aros/kernel.h>
#include <proto/exec.h>

#include <inttypes.h>
#include <string.h>


#include <kernel_cpu.h>
#include <kernel_debug.h>
#include <kernel_interrupts.h>
#include <kernel_objects.h>

/* We use own implementation of bug(), so we don't need aros/debug.h */
#define D(x)

/*****************************************************************************

            Register an Interrupt Controller in Kernelbase. Assign an ID (in ic_Id)
            returns KRN_ICINTR_INVALID on failure.

*****************************************************************************/

icintrid_t krnAddInterruptController(struct KernelBase *KernelBase, struct IntrController *intController)
{
    struct IntrController *regContr;
    icid_t icid = 0;

    ForeachNode(&KernelBase->kb_ICList, regContr)
    {
        if (!strcmp(intController->ic_Node.ln_Name, regContr->ic_Node.ln_Name))
        {
            /* Already registered, return its ID */
            regContr->ic_Count++;

            D(bug("[Kernel] %s: controller id #%d, use count %d\n", __func__, regContr->ic_Id, regContr->ic_Count));

            return ICINTR_MAKE(regContr->ic_Id, regContr->ic_Count);
        }
    }
    intController->ic_Count = 1;                                                                                  /* first user */
    intController->ic_Id = KernelBase->kb_ICTypeBase++;
    /*
     * ln_Type only has room for the low byte of the id. It is kept in step
     * for anything that reads the node directly, but ic_Id is the value.
     */
    intController->ic_Node.ln_Type = (UBYTE)intController->ic_Id;

    if (intController->ic_Register)
        icid = intController->ic_Register(KernelBase);
    else
        icid = intController->ic_Id;

    if (icid == KRN_ICID_INVALID)
        return KRN_ICINTR_INVALID;

    Enqueue(&KernelBase->kb_ICList, &intController->ic_Node);

    D(bug("[Kernel] %s: new controller id #%d = '%s'\n", __func__, intController->ic_Id, intController->ic_Node.ln_Name));

    return ICINTR_MAKE(icid, intController->ic_Count);
}

/*****************************************************************************/

struct IntrController *krnFindInterruptController(struct KernelBase *KernelBase, ULONG ICType)
{
    struct IntrController *intContr;
    ForeachNode(&KernelBase->kb_ICList, intContr)
    {
        if (intContr->ic_Type == ICType)
        {
            return intContr;
        }
    }
    return NULL;
}

/*****************************************************************************/

BOOL krnInitInterrupt(struct KernelBase *KernelBase, irqid_t irq, icid_t icid, icinstid_t icinstance)
{
    if (KERNELIRQ_ICID(irq) == KBL_INTERNAL)
    {
        KERNELIRQ_ICID(irq) = icid;
        KERNELIRQ_ICINST(irq) = icinstance;
        return TRUE;
    }
    return FALSE;
}

/* Returns a mapping node for a requested Device Interrupt */
struct IntrMapping *krnInterruptMapping(struct KernelBase *KernelBase, irqid_t irq)
{
    struct IntrMapping *intrMap;

    ForeachNode(&KernelBase->kb_InterruptMappings, intrMap)
    {
        if (intrMap->im_DeviceIRQ == irq)
        {
            return intrMap;
        }
    }
    return NULL;
}

/* Returns a mapping node for a requested controller Hardware Interrupt */
struct IntrMapping *krnInterruptMapped(struct KernelBase *KernelBase, ULONG irq)
{
    struct IntrMapping *intrMap;

    ForeachNode(&KernelBase->kb_InterruptMappings, intrMap)
    {
        if (intrMap->im_Int == irq)
        {
            return intrMap;
        }
    }
    return NULL;
}

/*****************************************************************************

            Initialize the registered Interrupt Controllers.
            
*****************************************************************************/

int krnInitInterruptControllers(struct KernelBase *KernelBase)
{
    struct IntrController *regContr;
    int cnt = 0;

    ForeachNode(&KernelBase->kb_ICList, regContr)
    {
        if (regContr->ic_Init)
        {
            if (regContr->ic_Init(KernelBase, regContr->ic_Count))
            {
                regContr->ic_Flags |= ICF_READY;
                cnt += regContr->ic_Count;
            }
        }
    }
    return cnt;
}

#endif
