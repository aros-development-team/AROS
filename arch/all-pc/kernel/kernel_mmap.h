#include <aros/multiboot.h>
#include <exec/lists.h>

/* This structure describes a single hardware memory region */
struct MemRegion
{
    IPTR   start;
    IPTR   end;
    STRPTR name;
    BYTE   pri;
    ULONG  flags;
};

#define ALLOCATOR_STD   1
#define ALLOCATOR_TLSF  2

void mmap_InitMemory(struct mb_mmap *mmap, unsigned long len, struct MinList *memList,
		     IPTR klo, IPTR khi, IPTR reserve, const struct MemRegion *reg, ULONG allocator);
struct mb_mmap *mmap_FindRegion(IPTR addr, struct mb_mmap *mmap, unsigned long len);
BOOL mmap_ValidateRegion(unsigned long addr, unsigned long len, struct mb_mmap *mmap, unsigned long mmap_len);
