/*
 * Kernel memory allocation functions
 *
 * In this default file they are just exec functions. However you may
 * reimplement them for a system with memory protection.
 *
 * void *krnAllocMem(unsigned int len);
 * void krnFreeMem(void *addr, unsigned int len);
 */

#include <exec/memory.h>
#include <proto/exec.h>

#define krnAllocMem(len) AllocMem(len, MEMF_PUBLIC|MEMF_CLEAR)
#define krnFreeMem(addr, len) FreeMem(addr, len)
