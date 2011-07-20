/*
 * Boot-time memory management functions.
 * In fact this is just a placeholder. All this can be seriously imprived.
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
    IPTR addr = AROS_ROUNDUP2(__KernBootPrivate->kbp_PrivateNext, align);

    __KernBootPrivate->kbp_PrivateNext = addr + size;
    /* Clear the allocated memory. In many places we expect it. */
    memset((void *)addr, 0, size);

    return (void *)addr;
}
