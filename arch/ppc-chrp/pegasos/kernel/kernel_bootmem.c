/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * Boot-time memory management functions.
 * In fact this is just a placeholder. All this can be seriously improved.
 */

#include <string.h>

#include "kernel_bootmem.h"
#include "kernel_intern.h"

void *krnAllocBootMem(unsigned int size)
{
    return krnAllocBootMemAligned(size, sizeof(void *));
}

void *krnAllocBootMemAligned(unsigned int size, unsigned int align)
{
    void *addr = (void *)AROS_ROUNDUP2(__BootData->bd_BootMem, align);

    __BootData->bd_BootMem = addr + size;
    /* Clear the allocated memory. In many places we expect it. */
    memset(addr, 0, size);

    return addr;
}
