/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/
#ifndef __EXEC_PLATFORM_H
#define __EXEC_PLATFORM_H

// needed to determine if this is an smp build..
#include <aros/config.h>

#if defined(__AROSEXEC_SMP__)
#include <aros/types/spinlock_s.h>

extern void Kernel_40_KrnSpinInit(spinlock_t *, void *);
#define EXEC_SPINLOCK_INIT(a,b) Kernel_40_KrnSpinInit(a,b)
#endif

#include "tls.h"

struct Exec_PlatformData
{
    /* No platform-specific data by default */
};

#define GET_THIS_TASK           TLS_GET(ThisTask)
#define SET_THIS_TASK(x)        TLS_SET(ThisTask,(x))

#endif /* __EXEC_PLATFORM_H */
