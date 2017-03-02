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

void core_APIC_RegisterMSI(void *handle)
{
    struct IntrNode *msihandle = (struct IntrNode *)handle;

    D(bug("[APIC:MSI] %s: MSI Handler @ 0x%p\n", msihandle));
    if (msihandle->in_nr >= APIC_MSI_BASE)
    {
        
    }
}
