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

        data->cores[0].cpu_GDT = __KernBootPrivate->BOOTGDT;
        data->cores[0].cpu_TLS = __KernBootPrivate->BOOTTLS;
        data->cores[0].cpu_IDT = __KernBootPrivate->BOOTIDT;
        data->cores[0].cpu_MMU = &__KernBootPrivate->MMU;

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

    D(bug("[APIC] %s()\n", __func__));

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

    D(bug("[APIC] %s()\n", __func__));
   
    if (!data)
    {
        D(bug("[APIC] %s: BSP or uniprocessor\n", __func__));
        /* No APIC data -> uniprocessor system */
    	return 0;
    }

    __APICLogicalID = core_APIC_GetID(data->lapicBase);

    D(bug("[APIC] %s: APIC ID %03u\n", __func__, __APICLogicalID));

    return core_APIC_GetNumberFromLocal(data, __APICLogicalID);
}

void core_APIC_GetMask(struct APICData *data, apicid_t cpuNo, cpumask_t *mask)
{
    ULONG *apicMask;
    int idlong, idbit;

    D(bug("[APIC] %s()\n", __func__));

    if ((IPTR)mask != TASKAFFINITY_ANY)
    {
        idlong = cpuNo / 32;
        idbit = cpuNo - (idlong * 32); 

        D(bug("[APIC] %s: %d -> %d:%d\n", __func__, cpuNo, idlong, idbit));
        D(bug("[APIC] %s: mask @ 0x%p\n", __func__, mask));

        if ((apicMask = (ULONG *)mask) != NULL)
            apicMask[idlong] = (1 << idbit);
    }

    return;
}

BOOL core_APIC_CPUInMask(apicid_t cpuNo, cpumask_t *mask)
{
    ULONG *apicMask;
    int idlong, idbit;

    D(bug("[APIC] %s()\n", __func__));

    if (mask == NULL)
    {
        if (cpuNo == 0)
            return TRUE;
        return FALSE;
    }

    if ((IPTR)mask == TASKAFFINITY_ANY)
    {
        // TODO: make sure it is a valid cpu number in the range of available cpus.
        return TRUE;
    }

    idlong = cpuNo / 32;
    idbit = cpuNo - (idlong * 32); 

    D(bug("[APIC] %s: %d -> %d:%d\n", __func__, cpuNo, idlong, idbit));
    D(bug("[APIC] %s: mask @ 0x%p\n", __func__, mask));

    if (((apicMask = (ULONG *)mask) != NULL) && (apicMask[idlong] & (1 << idbit)))
        return TRUE;

    return FALSE;
}
