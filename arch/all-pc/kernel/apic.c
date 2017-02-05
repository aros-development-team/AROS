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
    APTR ssp;

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

    D(bug("[APIC] APIC Data @ 0x%p\n", data));

    if ((ssp = SuperState()) != NULL)
    {
        data->lapicBase	   = core_APIC_GetBase();
        
        D(bug("[APIC] APIC Base = 0x%p\n", data->lapicBase));

        data->apic_count		   = 1;
        data->flags		   = APF_8259;
        data->cores[0].cpu_LocalID = core_APIC_GetID(data->lapicBase);

        UserState(ssp);

        D(bug("[APIC] ID #%d\n", data->cores[0].cpu_LocalID));

#if (__WORDSIZE==64)
        data->cores[0].cpu_GDT = __KernBootPrivate->BOOTGDT;
        data->cores[0].cpu_IDT = __KernBootPrivate->BOOTIDT;
        data->cores[0].cpu_MMU = &__KernBootPrivate->MMU;
#endif

        /* Just initialize to default state */
        core_APIC_Init(data, 0);
    }
    else
    {
        FreeMem(data, sizeof(struct APICData) + sizeof(struct CPUData));
        data = NULL;
    }
    return data;
}

apicid_t core_APIC_GetNumberFromLocal(struct APICData *data, apicid_t apicLocalID)
{
    apicid_t __APICNo;

    if (!data)
    {
        /* No APIC data -> uniprocessor system */
        if (apicLocalID > 0)
            return -1;
        else
            return 0;
    }

    for (__APICNo = 0; __APICNo < data->apic_count; __APICNo++)
    {
        if (data->cores[__APICNo].cpu_LocalID == apicLocalID)
            return __APICNo;
    }

    return -1;
}

apicid_t core_APIC_GetNumber(struct APICData *data)
{
    apicid_t __APICLogicalID;

    if (!data)
    {
        /* No APIC data -> uniprocessor system */
    	return 0;
    }

    __APICLogicalID = core_APIC_GetID(data->lapicBase);

    return core_APIC_GetNumberFromLocal(data, __APICLogicalID);
}

uint32_t core_APIC_GetMask(struct APICData *data, apicid_t cpuNo)
{
    if (!data)
    {
        /* No APIC data -> uniprocessor system */
    	return (1 << 0);
    }

    return (1 << data->cores[cpuNo].cpu_LocalID);
}
