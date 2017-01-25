/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id: addexceptionhandler.c 47708 2013-07-17 12:22:32Z verhaegs $

    Desc:
*/

#include <aros/kernel.h>
#include <proto/exec.h>

#include <inttypes.h>
#include <string.h>

#include <kernel_base.h>
#include <kernel_cpu.h>
#include <kernel_debug.h>
#include <kernel_interrupts.h>
#include <kernel_objects.h>

/* We use own implementation of bug(), so we don't need aros/debug.h */
#define D(x) 

/*****************************************************************************/

icintrid_t krnAddInterruptController(struct KernelBase *KernelBase, struct IntrController *intController)
{
    struct IntrController *regContr;
    icid_t icid = 0;
    icid_t icinstance = 0;
    
    ForeachNode(&KernelBase->kb_ICList, regContr)
    {
        if (!strcmp(intController->ic_Node.ln_Name, regContr->ic_Node.ln_Name))
        {
            /* Already registered, return its ID */
            regContr->ic_Node.ln_Pri++;
            
            D(bug("[Kernel] %s: controller id #%d, use count %d\n", __func__, regContr->ic_Node.ln_Type, regContr->ic_Node.ln_Pri));

            return (icintrid_t)((regContr->ic_Node.ln_Type << 8) | regContr->ic_Node.ln_Pri);
        }
    }
    intController->ic_Node.ln_Pri = 1;                                                                                  /* first user */
    intController->ic_Node.ln_Type = KernelBase->kb_ICTypeBase++;
    AddTail(&KernelBase->kb_ICList, &intController->ic_Node);

    D(bug("[Kernel] %s: new controller id #%d = '%s'\n", __func__, intController->ic_Node.ln_Type, intController->ic_Node.ln_Name));

    icid = intController->ic_Node.ln_Type;

    if (intController->ic_Register)
        icid = intController->ic_Register(KernelBase);

    return (icintrid_t)((icid << 8) | icinstance);
}

int krnInitInterruptControllers(struct KernelBase *KernelBase)
{
    struct IntrController *regContr;
    int cnt = 0;

    ForeachNode(&KernelBase->kb_ICList, regContr)
    {
        if (regContr->ic_Init)
        {
            if (regContr->ic_Init(KernelBase, regContr->ic_Node.ln_Pri))
                cnt += regContr->ic_Node.ln_Pri;
        }
    }
    return cnt;
}
