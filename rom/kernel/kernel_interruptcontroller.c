/*
    Copyright © 2017-2020, The AROS Development Team. All rights reserved.
    $Id$

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

            Register an Interrupt Controller in Kernelbase. Assign an ID (in ln_Type)
            returns -1 on failure.
            
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
            
            D(bug("[Kernel] %s: controller id #%d, use count %d\n", __func__, regContr->ic_Node.ln_Type, regContr->ic_Count));

            return (icintrid_t)((regContr->ic_Node.ln_Type << 8) | regContr->ic_Count);
        }
    }
    intController->ic_Count = 1;                                                                                  /* first user */
    intController->ic_Node.ln_Type = KernelBase->kb_ICTypeBase++;

    if (intController->ic_Register)
        icid = intController->ic_Register(KernelBase);
    else
        icid = intController->ic_Node.ln_Type;

    if (icid == (icid_t)-1)
        return (icintrid_t)-1;

    Enqueue(&KernelBase->kb_ICList, &intController->ic_Node);

    D(bug("[Kernel] %s: new controller id #%d = '%s'\n", __func__, intController->ic_Node.ln_Type, intController->ic_Node.ln_Name));

    return (icintrid_t)((icid << 8) | intController->ic_Count);
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

BOOL krnInitInterrupt(struct KernelBase *KernelBase, icid_t irq, icid_t icid, icid_t icinstance)
{
    if (KERNELIRQ_LIST(irq).lh_Type == KBL_INTERNAL)
    {
        KERNELIRQ_LIST(irq).lh_Type = icid;
        KERNELIRQ_LIST(irq).l_pad = icinstance;
        return TRUE;
    }
    return FALSE;
}

/* Returns a mapping node for a requested Device Interrupt */
struct IntrMapping *krnInterruptMapping(struct KernelBase *KernelBase, icid_t irq)
{
    struct IntrMapping *intrMap;

    ForeachNode(&KernelBase->kb_InterruptMappings, intrMap)
    {
        if (intrMap->im_Node.ln_Pri == irq)
        {
            return intrMap;
        }
    }
    return NULL;
}

/* Returns a mapping node for a requested controller Hardware Interrupt */
struct IntrMapping *krnInterruptMapped(struct KernelBase *KernelBase, icid_t irq)
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
