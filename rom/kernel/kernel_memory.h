#include <exec/memory.h>
#include <proto/exec.h>

#define AllocKernelMem(len) AllocMem(len, MEMF_PUBLIC|MEMF_CLEAR)
#define FreeKernelMem(addr, len) FreeMem(addr, len)
