/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/asmcall.h>
#include <proto/arossupport.h>
#include <proto/acpica.h>

#include <inttypes.h>
#include <string.h>

#include "kernel_base.h"
#include "kernel_bootmem.h"
#include "kernel_debug.h"
#include "kernel_globals.h"
#include "kernel_intern.h"
#include "apic.h"

#define D(x) x
   
/* This must be global, for acpica.library is a rellib
 */
struct Library *ACPICABase;

ULONG acpi_Initialize(void)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
        
    ACPICABase = OpenLibrary("acpica.library", 0);

    D(bug("[Kernel] core_ACPIInitialise()\n"));

    if (!ACPICABase)
    {
    	D(bug("[Kernel] acpica.library not found, no ACPICA\n"));
    	return 0;
    }

    /*
     * ACPI exists. Parse all the data.
     * Currently we only initialize local APIC.
     */
    pdata->kb_APIC = acpi_APIC_Init(ACPICABase);

    return 1;
}
