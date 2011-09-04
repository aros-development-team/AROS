/*
 * Boot-time memory management functions.
 * In fact this is just a placeholder. All this can be seriously improved.
 */

#include <aros/macros.h>
#include <string.h>

#include "boot_utils.h"
#include "kernel_bootmem.h"

void *krnAllocBootMem(unsigned long size)
{
    return krnAllocBootMemAligned(size, sizeof(void *));
}

void *krnAllocBootMemAligned(unsigned long size, unsigned int align)
{
    void *addr = (void *)AROS_ROUNDUP2((unsigned long)BootMemPtr, align);
    void *end = addr + size;

    /* TODO: Implement limit check */
    
    BootMemPtr = end;
    /* Clear the allocated memory. In many places we expect it. */
    memset(addr, 0, size);

    return addr;
}

void *BootMemPtr;
