/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>

void *krnAllocMemAligned(unsigned long size, unsigned long align)
{
    void *mem;

    align--;
    mem = AllocMem(size + align, MEMF_PUBLIC|MEMF_CLEAR);
    if (!mem)
	return NULL;

    return (void *)(((IPTR)mem + align) & (~align));
}
