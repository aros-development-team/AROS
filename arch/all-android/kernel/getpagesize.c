/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Query Android host OS for memory page size
    Lang: english
*/

#include "hostinterface.h"
#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_unix.h"

unsigned int krnGetPageSize(void *hostlib)
{
    unsigned int *__page_size;
    char *errstr;

    __page_size = HostIFace->hostlib_GetPointer(hostlib, "__page_size", &errstr);

    if (!__page_size)
    {
    	krnPanic(NULL, "Failed to obtain memory page size\n"
    		       "%s", errstr);
    	return 0;
    }

    return *__page_size;
}
