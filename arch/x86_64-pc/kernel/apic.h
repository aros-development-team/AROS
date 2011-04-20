/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS APIC Definitions.
    Lang: english
*/

#ifndef __AROS_APIC_H__
#define __AROS_APIC_H__

#include <exec/lists.h>
#include <exec/semaphores.h>
#include <utility/hooks.h>

#define	APIC_DEFAULT_PHYS_BASE    0xfee00000
#ifndef MAX_IO_APICS
#define MAX_IO_APICS 32
#endif

/********** APIC DEFINITIONS ****************/

struct GenericAPIC
{ 
	const char                   *name;
	IPTR                        (*probe)(const struct GenericAPIC *apic, struct KernBootPrivate *bootdata);
        IPTR                        (*getbase)(void);
        IPTR                        (*getid)(IPTR base);
	IPTR                        (*wake)(APTR startrip, UBYTE apicid, struct PlatformData *data);
	IPTR                        (*init)(IPTR base);
	IPTR                        (*apic_id_registered)();
};

IPTR core_APICProbe(struct KernBootPrivate *__KernBootPrivate);
UBYTE core_APICGetNumber(struct PlatformData *pdata);

/* Driver call stubs */
static inline void core_APIC_AckIntr(uint8_t intnum, struct PlatformData *pd)
{
    asm volatile ("movl %0,(%1)"::"r"(0),"r"(pd->kb_APIC_BaseMap[0] + 0xb0));
}

static inline IPTR core_APIC_Wake(APTR start_addr, UBYTE id, struct PlatformData *pdata)
{
    return pdata->kb_APIC_Drivers[pdata->kb_APIC_DriverID]->wake(start_addr, id, pdata);
}

static inline IPTR core_APIC_GetBase(struct PlatformData *pd)
{
    return pd->kb_APIC_Drivers[pd->kb_APIC_DriverID]->getbase();
}

static inline IPTR core_APIC_Init(struct PlatformData *pd)
{
    return pd->kb_APIC_Drivers[pd->kb_APIC_DriverID]->init(pd->kb_APIC_BaseMap[0]);
}

#endif /* __AROS_APIC_H__ */
