#include <aros/multiboot.h>
#include <exec/memory.h>
#include <proto/exec.h>

/* This structure describes a single hardware memory region */
struct MemRegion
{
    IPTR   start;
    IPTR   end;
    STRPTR name;
    BYTE   pri;
    ULONG  flags;
};

void mmap_InitMemory(struct mb_mmap *mmap, unsigned long len, struct MinList *memList,
		     IPTR klo, IPTR khi, const struct MemRegion *reg);

/* We use common exec memory allocator */
#define krnAllocMem(len) AllocMem(len, MEMF_PUBLIC|MEMF_CLEAR)
#define krnFreeMem(addr, len) FreeMem(addr, len)
