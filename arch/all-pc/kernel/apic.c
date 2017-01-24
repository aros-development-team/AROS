/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/memory.h>
#include <proto/exec.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "apic.h"

#define D(x)

/* Initialize APIC by probing */
struct APICData *core_APIC_Probe(void)
{
    struct APICData *data;
#ifdef __i386__
    /* Only i386 needs detection. On x86-64 APIC is always present. */
    ULONG arg = 1;
    ULONG res;

    asm volatile("cpuid":"+a"(arg),"=d"(res)::"%ebx", "%ecx");
    
    if (!(res & (1 << 9)))
    	return NULL;

    D(bug("[APIC] Detected by CPUID instruction\n"));
#endif

    /* Assuming uniprocessor IBM PC compatible */
    data = AllocMem(sizeof(struct APICData) + sizeof(struct CPUData), MEMF_CLEAR);
    if (!data)
	return NULL;

    data->lapicBase	   = core_APIC_GetBase();
    data->apic_count		   = 1;
    data->flags		   = APF_8259;
    data->cores[0].lapicID = core_APIC_GetID(data->lapicBase);

    /* Just initialize to default state */
    core_APIC_Init(data, 0);
    return data;
}

apicid_t core_APIC_GetNumber(struct APICData *data)
{
    apicid_t __APICLogicalID;
    apicid_t __APICNo;

    if (!data)
    {
        /* No APIC data -> uniprocessor system */
    	return 0;
    }

    __APICLogicalID = core_APIC_GetID(data->lapicBase);

    for (__APICNo = 0; __APICNo < data->apic_count; __APICNo++)
    {
        if (data->cores[__APICNo].lapicID == __APICLogicalID)
            return __APICNo;
    }

    return -1;
}
