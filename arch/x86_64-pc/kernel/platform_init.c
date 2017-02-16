/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __KERNEL_NOLIBBASE__

#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <exec/lists.h>
#include <proto/exec.h>

#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_debug.h"

#define D(x)
#define DAPIC(x)

/* 
    This file contains code that is run once Exec has been brought up - and is launched
    via the RomTag/Autoinit routines in Exec.
    
    Here we do the platform setup that requires Exec to have minimal configuration
    but still running inthe SINGLETASK environment.
*/

extern struct syscallx86_Handler x86_SCSupervisorHandler;

static int Platform_Init(struct KernelBase *LIBBASE)
{
    struct PlatformData *pdata;
    int i;

    D(
        bug("[Kernel:x86_64] %s: Performing Post-Exec initialization\n", __func__);
        bug("[Kernel:x86_64] %s: KernelBase @ %p\n", __func__, LIBBASE);
    )

    /*
     * Setup the Interrupt Controller Environment ...
     */
    NEWLIST(&LIBBASE->kb_ICList);
    NEWLIST(&LIBBASE->kb_InterruptMappings);
    LIBBASE->kb_ICTypeBase = KBL_INTERNAL + 1;

    D(bug("[Kernel:x86_64] %s: Interrupt Controller Base ID = %d\n", __func__, LIBBASE->kb_ICTypeBase));

    for (i = 0; i < HW_IRQ_COUNT; i++)
    {
        if (i == APIC_IRQ_SYSCALL)
        {
            LIBBASE->kb_Interrupts[i].ki_Priv |= IRQINTF_ENABLED;               // Reserve the Syscall Handler..
            LIBBASE->kb_Interrupts[i].ki_List.lh_Type = KBL_INTERNAL - 1;
        }
        else
        {
            LIBBASE->kb_Interrupts[i].ki_Priv &= ~IRQINTF_ENABLED;
            LIBBASE->kb_Interrupts[i].ki_List.lh_Type = KBL_INTERNAL;
        }
    }

    D(bug("[Kernel:x86_64] %s: Interrupt Lists initialised\n", __func__));

    /*
     * Setup our Platform Data ...
     */
    pdata = AllocMem(sizeof(struct PlatformData), MEMF_PUBLIC|MEMF_CLEAR);
    if (!pdata)
    	return FALSE;

    D(bug("[Kernel:x86_64] %s: Platform Data allocated @ 0x%p\n", __func__, pdata));

    LIBBASE->kb_PlatformData = pdata;

    /*
     * Setup the base syscall handler(s) ...
     */
    NEWLIST(&pdata->kb_SysCallHandlers);

    // we need to setup the BSP's syscall gate early..
    if (!core_SetIDTGate((apicidt_t *)__KernBootPrivate->BOOTIDT, APIC_IRQ_SYSCALL, (uintptr_t)IntrDefaultGates[APIC_IRQ_SYSCALL], TRUE))
    {
        krnPanic(NULL, "Failed to set BSP Syscall Vector\n"
                       "Vector #%02X\n", APIC_IRQ_SYSCALL);
    }
    krnAddSysCallHandler(pdata, &x86_SCSupervisorHandler, FALSE, TRUE);

    return TRUE;
}

ADD2INITLIB(Platform_Init, 10)
