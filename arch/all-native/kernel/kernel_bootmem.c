/*
 * Boot-time memory management functions.
 * This is a very simple allocator working on a continuous memory block.
 * Its purpose is to help to set up initial boot-time data for your kernel,
 * until it can do more serious thing.
 * A popular usage is to store away boot information.
 */

#include <aros/macros.h>
#include <string.h>

#include "kernel_base.h"
#include "kernel_bootmem.h"
#include "kernel_debug.h"

void *krnAllocBootMem(unsigned long size)
{
    return krnAllocBootMemAligned(size, sizeof(void *));
}

void *krnAllocBootMemAligned(unsigned long size, unsigned int align)
{
    void *addr = (void *)AROS_ROUNDUP2((unsigned long)BootMemPtr, align);
    void *end = addr + size;

#ifdef __x86_64__
    /* FIXME: Only x86-64 currently sets BootMemLimit */
    if (end > BootMemLimit)
    {
	/* Our allocation must succeed. We can't continue without it. */
    	krnPanic(NULL, "Not enough memory for boot information\n"
    		       "Increase reserved space in bootstrap");
    }
#endif

    BootMemPtr = end;
    /* Clear the allocated memory. In many places we expect it. */
    memset(addr, 0, size);

    return addr;
}

void *BootMemPtr;
void *BootMemLimit;
