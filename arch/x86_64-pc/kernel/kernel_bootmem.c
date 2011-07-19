/*
 * Boot-time memory management functions.
 * In fact this is just a placeholder. All this can be seriously imprived.
 */

#include "kernel_bootmem.h"
#include "kernel_intern.h"

void *krnAllocBootMem(unsigned int size)
{
    IPTR addr = AROS_ROUNDUP2(__KernBootPrivate->kbp_PrivateNext, sizeof(void *));

    __KernBootPrivate->kbp_PrivateNext = addr + size;

    return (void *)addr;
}
