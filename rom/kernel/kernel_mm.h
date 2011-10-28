#include <exec/memory.h>

#include <inttypes.h>

/*
 * The following functions operate on 'barebone' memory allocator.
 * They assume you have appropriate privileges for the allocator to work.
 * They won't change access rights of the memory they operate on.
 *
 * This two-level design allows safe boot-time memory initialization.
 * MMU control structures take up (lots of) memory themselves, and you
 * need to know where to place them.
 * To solve this, you first initialize memory allocator, and then allocate
 * memory for MMU control structures. After the MMU is up, you can manipulate
 * protection attributes.
 */
void *mm_AllocPages(void *addr, uintptr_t length, uint32_t flags, struct KernelBase *KernelBase);
void mm_FreePages(void *addr, uintptr_t length, struct KernelBase *KernelBase);

/*
 * Low-level functions, implemented by the allocator.
 * They will be different, depending on the allocator implementation.
 */
APTR mm_Allocate(struct MemHeader *mh, IPTR size, ULONG flags);
APTR mm_AllocAbs(struct MemHeader *mh, void *addr, IPTR size);
void mm_Free(struct MemHeader *mh, APTR addr, IPTR size);
void mm_StatMemHeader(struct MemHeader *mh, const struct TagItem *query, struct KernelBase *KernelBase);
void mm_Init(struct MemHeader *mh, ULONG pageSize);
