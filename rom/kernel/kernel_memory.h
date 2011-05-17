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

/*
 * 'access' parameter in krnAllocMem() is reserved to provide
 * user-mode access flags. When set to '0' the memory is supposed to be
 * accessible only in supervisor mode.
 */

#define krnAllocMem(len, access) AllocMem(len, MEMF_PUBLIC|MEMF_CLEAR)
#define krnFreeMem(addr, len) FreeMem(addr, len)
