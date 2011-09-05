/*
 * Boot-time memory management functions.
 * This is a very simple allocator working on a continuous memory block.
 * Its purpose is to help to set up initial boot-time data for your kernel,
 * until it can do more serious thing.
 * A popular usage is to store away boot information.
 */

#include <aros/macros.h>
#include <string.h>

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
void *BootMemLimit;
