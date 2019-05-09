
#define MMU030 1
#define MMU040 2
#define MMU060 3

#define CM_WRITETHROUGH 0
#define CM_COPYBACK 1
#define CM_SERIALIZED 2
#define CM_NONCACHEABLE 3

/* Platform-specific part of KernelBase */
struct PlatformData
{
    ULONG *MMU_Level_A;
    UBYTE mmu_type;
    UBYTE *page_ptr;
    ULONG page_free;
    UBYTE *zeropagedescriptor;
    UBYTE cachemodestore; /* CachePreDMA()/CachePostDMA() */

    /*
     * romimg points to an array of virt->phys address mappings
     * for an m68k systems rom images (if used).
     * e.g
     *   rom_a_virt, rom_a_phys,
     *   rom_b_virt, rom_b_phys,
     *   NULL, NULL
     */
    ULONG romsize;
    APTR *romimg;
};

extern BOOL map_region(struct KernelBase *kb, void *addr, void *physaddr, ULONG size, BOOL invalid, BOOL writeprotect, BOOL supervisor, UBYTE cachemode);
extern BOOL unmap_region(struct KernelBase *kb, void *addr, ULONG size);
extern void debug_mmu(struct KernelBase *kb);
extern void enable_mmu(struct KernelBase *kb);
extern void disable_mmu(struct KernelBase *kb);
extern BOOL init_mmu(struct KernelBase *kb);

