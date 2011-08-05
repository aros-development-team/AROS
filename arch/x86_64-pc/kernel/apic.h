/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS APIC functions.
    Lang: english
*/

#ifndef __AROS_APIC_H__
#define __AROS_APIC_H__

#include <utility/hooks.h>

#include "apic_driver.h"

UBYTE core_APIC_GetNumber(struct PlatformData *pdata, IPTR __APICBase);

/* Driver call stubs */

static inline IPTR core_APIC_Wake(APTR start_addr, UBYTE id, IPTR base)
{
    return __KernBootPrivate->kbp_APIC_Driver->wake(start_addr, id, base);
}

static inline IPTR core_APIC_GetBase(void)
{
    return __KernBootPrivate->kbp_APIC_Driver->getbase();
}

static inline IPTR core_APIC_GetID(IPTR base)
{
    return __KernBootPrivate->kbp_APIC_Driver->getid(base);
}

static inline IPTR core_APIC_Init(IPTR base)
{
    return __KernBootPrivate->kbp_APIC_Driver->init(base);
}

static inline void core_APIC_AckIntr(UBYTE num)
{
    __KernBootPrivate->kbp_APIC_Driver->ack(num);
}

#endif /* __AROS_APIC_H__ */
