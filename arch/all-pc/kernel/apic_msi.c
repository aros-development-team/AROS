/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_cpu.h>
#include <kernel_debug.h>
#include <kernel_interrupts.h>
#include <kernel_objects.h>

#define D(x)

ULONG core_APIC_AllocMSI()
{
    D(bug("[APIC:MSI] %s()\n"));
    return (ULONG)-1;
}

void core_APIC_RegisterMSI(void *handle)
{
    struct IntrNode *msihandle = (struct IntrNode *)handle;

    D(bug("[APIC:MSI] %s: MSI Handler @ 0x%p\n", msihandle));
    if ((msihandle->in_nr >= APIC_MSI_BASE) && (msihandle->in_nr < (ULONG)-1))
    {
        D(bug("[APIC:MSI] %s: Registering MSI %d\n", (int)msihandle->in_nr));
    }
}
