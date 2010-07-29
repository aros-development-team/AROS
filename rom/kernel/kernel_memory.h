#include <exec/memory.h>
#include <proto/exec.h>

#define krnAllocMem(len) AllocMem(len, MEMF_PUBLIC|MEMF_CLEAR)
#define krnFreeMem(addr, len) FreeMem(addr, len)
