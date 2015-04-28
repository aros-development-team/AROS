/*
 * This file can be overriden in arch/all-$(ARCH)/exec. Currently
 * used only by Windows-hosted port
 */
#ifndef __EXEC_PLATFORM_H
#define __EXEC_PLATFORM_H

//#define __AROSEXEC_SMP__

#include "tls.h"

struct Exec_PlatformData
{
    /* No platform-specific data by default */
};

#define GET_THIS_TASK           TLS_GET(ThisTask)
#define SET_THIS_TASK(x)        TLS_SET(ThisTask,(x))

#endif /* __EXEC_PLATFORM_H */
