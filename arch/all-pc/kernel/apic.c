/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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
    ULONG res;

    asm volatile("cpuid":"=d"(res):"a"(1):"eax", "ebx", "ecx", "edx");
    
    if (!(res & (1 << 9)))
    	return NULL;

    D(bug("[APIC] Detected by CPUID instruction\n"));
#endif

    data = AllocMem(sizeof(struct APICData) + sizeof(UWORD), MEMF_ANY);
    if (!data)
	return NULL;

    /* Assuming uniprocessor IBM PC compatible */
    data->lapicBase = core_APIC_GetBase();
    data->count     = 1;
    data->flags     = APF_8259;
    data->IDMap[0]  = core_APIC_GetID(data->lapicBase);

    /* Just initialize to default state */
    core_APIC_Init(data->lapicBase);
    return data;
}

UBYTE core_APIC_GetNumber(struct APICData *data)
{
    UBYTE __APICLogicalID;
    UBYTE __APICNo;

    if (!data)
    {
        /* No APIC data -> uniprocessor system */
    	return 0;
    }

    __APICLogicalID = core_APIC_GetID(data->lapicBase);

    for (__APICNo = 0; __APICNo < data->count; __APICNo++)
    {
        if ((data->IDMap[__APICNo] & 0xFF) == __APICLogicalID)
            return __APICNo;
    }

    return -1;
}
