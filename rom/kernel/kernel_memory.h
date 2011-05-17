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
 * user-mode access flags. In supervisor mode we assume to always
 * have RW access to the allocated memory. Also, current code assumes
 * krnAllocMem() will clear the memory (at least KrnCreateContext()
 * implementations and kernel.resource init code). Perhaps this is not good.
 */

#define krnAllocMem(len, access) AllocMem(len, MEMF_PUBLIC|MEMF_CLEAR)
#define krnFreeMem(addr, len) FreeMem(addr, len)
