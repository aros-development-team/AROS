/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/asmcall.h>
#include <resources/acpi.h>
#include <proto/arossupport.h>
#include <proto/acpi.h>

#include <inttypes.h>
#include <string.h>

#include "kernel_base.h"
#include "kernel_bootmem.h"
#include "kernel_debug.h"
#include "kernel_globals.h"
#include "kernel_intern.h"
#include "apic.h"

#define D(x) x

ULONG acpi_Initialize(void)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct ACPIBase *ACPIBase = OpenResource("acpi.resource");

    D(bug("[Kernel] core_ACPIInitialise()\n"));

    if (!ACPIBase)
    {
    	D(bug("[Kernel] acpi.resource not found, no ACPI\n"));
    	return 0;
    }

    /*
     * ACPI exists. Parse all the data.
     * Currently we only initialize local APIC.
     */
    pdata->kb_APIC = acpi_APIC_Init(ACPIBase);

    return 1;
}
