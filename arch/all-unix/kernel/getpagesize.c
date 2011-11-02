/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id: kernel_mm.c 42198 2011-11-02 10:53:45Z sonic $

    Desc: Query UNIX host OS for memory page size
    Lang: english
*/

#include "hostinterface.h"
#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_unix.h"

unsigned int krnGetPageSize(struct HostInterface *HostIFace, void *hostlib)
{
    int (*getpagesize)(void);
    char *errstr;
    int ret;

    getpagesize = HostIFace->hostlib_GetPointer(hostlib, "getpagesize", &errstr);
    AROS_HOST_BARRIER
    
    if (!getpagesize)
    {
    	krnPanic(NULL, "Failed to obtain memory page size\n"
    		       "%s", errstr);
    	return 0;
    }

    ret = getpagesize();
    AROS_HOST_BARRIER

    return ret;
}
