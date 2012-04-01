#include <aros/debug.h>
#include <asm/amcc440.h>
#include <asm/io.h>
#include <aros/kernel.h>

#include "kernel_intern.h"

static long tlb_bitmap[2] = {0xffffffff, 0xffffffff};
static long tlb_free      = 64;

/* Alloc TLB in the bitmap. Returns -1 if the allocation cannot be done */
static int alloc_tlb()
{
    /* It should be done in locked state only! */
    
    int bit = __builtin_clz(tlb_bitmap[0]);
    if (bit == 32)
    {
        bit += __builtin_clz(tlb_bitmap[1]);
    }
    else
    {
        tlb_bitmap[0] &= ~(0x80000000 >> bit);
    }
    if (bit == 64)
    {
        return -1;
    }
    else
    {
        tlb_bitmap[1] &= ~(0x80000000 >> (bit-32));
    }
    tlb_free--;
    return bit;
}

#if 0
static void free_tlb(int entry)
{
    if (entry >=0 && entry < 32)
    {
        if (tlb_bitmap[0] & (0x80000000 >> entry))
        {
            D(bug("[KRN] Freeing already free TLB!!!\n"));
        }
        else
        {
            tlb_bitmap[0] |= (0x80000000 >> entry);
            tlb_free++;
        }
    }
    else if (entry < 64)
    {
        entry -= 32;
        if (tlb_bitmap[1] & (0x80000000 >> entry))
        {
            D(bug("[KRN] Freeing already free TLB!!!\n"));
        }
        else
        {
            tlb_bitmap[1] |= (0x80000000 >> entry);
            tlb_free++;
        }
    }
    else
    {
        D(bug("[KRN] Wrong TLB\n"));
    }
}
#endif

static struct mmu_page_size {
    uint8_t     code;
    uintptr_t   mask;
} allowable_pages[] = {
        { 0x90, 0x0fffffff },   /* 256MB */
        { 0x70, 0x00ffffff },   /* 16MB */
        { 0x50, 0x000fffff },   /* 1MB */
        { 0x40, 0x0003ffff },   /* 256KB */
        { 0x30, 0x0000ffff },   /* 64KB */
        { 0x20, 0x00003fff },   /* 16KB */
        { 0x10, 0x00000fff },   /* 4KB */
        { 0x00, 0x000003ff },   /* 1KB */
        { 0xff, 0xffffffff },   /* END MARKER */
};

void map_region(uintptr_t physbase, uintptr_t virtbase, uintptr_t length, uint32_t prot)
{
    long tlb_temp = tlb_free;
    
    D(bug("[KRN] map_region(%08x, %08x, %08x, %04x): ", physbase, virtbase, length, prot));
    
    /* While there is still something to map */
    while (length)
    {
        int tlb;
        int i = 0;
        
        /* Check all available page sizes and try to match the best (the biggest) usable TLB entry */
        while (allowable_pages[i].code != 0xff)
        {
            if ((length > allowable_pages[i].mask) && !(physbase & allowable_pages[i].mask) && !(virtbase & allowable_pages[i].mask))
                break;
            i++;
        }
        
        if (allowable_pages[i].code == 0xff)
        {
            D(bug("\n[KRN] map_region failed\n"));
            return;
        }
        
        /* get free TLB */
        tlb = alloc_tlb();
        if (tlb == -1)
        {
            D(bug("\n[KRN] map_region: No more free TLB entries\n"));
            return;
        }
        
        //D(bug("\n[KRN] TLB%02x: %08x - %08x : %08x - %08x: ", tlb,
              //physbase, physbase + allowable_pages[i].mask,
              //virtbase, virtbase + allowable_pages[i].mask));
        
        /* Do really write to the tlb */
        asm volatile("tlbwe %0,%3,0; tlbwe %1,%3,1; tlbwe %2,%3,2"
                     ::"r"(virtbase | allowable_pages[i].code | TLB_V), "r"(physbase), "r"(prot), "r"(tlb));
        //D(bug("%08x %08x %08x ", virtbase | allowable_pages[i].code | 0x200, physbase, prot));
        
        length -= allowable_pages[i].mask + 1;
        physbase += allowable_pages[i].mask + 1;
        virtbase += allowable_pages[i].mask + 1;
        
    }
    tlb_temp -= tlb_free;
    D(bug("%2d TLB%s\n", tlb_temp, tlb_temp > 1 ? "s":""));
}

void mmu_init(struct TagItem *tags)
{
    uintptr_t krn_lowest  = krnGetTagData(KRN_KernelLowest,  0, tags);
    uintptr_t krn_highest = krnGetTagData(KRN_KernelHighest, 0, tags);
    uintptr_t krn_base    = krnGetTagData(KRN_KernelBase,    0, tags);
    
    D(bug("[KRN] MMU Init\n"));
    D(bug("[KRN] lowest = %p, base = %p, highest = %p\n", krn_lowest, krn_base, krn_highest));
    D(bug("[KRN] Kernel size: %dKB code, %dKB data\n", (krn_highest - krn_base)/1024, (krn_base - krn_lowest)/1024));

    /*
     * In order to reduce the usage of TLB entries, align the kernel regions.
     * It wastes a tiny bit of RAM but saves a lot of TLB entries.
     */

    /* 4K granularity for data sections */
    krn_lowest &= 0xfffff000;
    /* 64K granularity for code sections */
    krn_highest = (krn_highest + 0xffff) & 0xffff0000;
    
    /* 
     * The very first entry has to cover the executable part of kernel, 
     * where exception handlers are located
     */
    map_region(krn_base, 0xff000000 + krn_base, krn_highest - krn_base, TLB_SR | TLB_SX | TLB_UR | TLB_UX);
    /* Now the data area for kernel. Make it read/write for both user and supervisor. No execution allowed */
    map_region(krn_lowest, 0xff000000 + krn_lowest, krn_base - krn_lowest, TLB_SR | TLB_SW | TLB_UR | TLB_UW);
    /* The low memory will be RW assigned to the supervisor mode. No access from usermode! */
    map_region(0, 0xff000000, krn_lowest, TLB_SR | TLB_SW);
    
    /* The regular RAM, make 1GB of it - amcc440 cannot do more. */
    map_region(krn_highest, krn_highest, 0x40000000 - krn_highest, TLB_SR | TLB_SW | TLB_UR | TLB_UW | TLB_SX | TLB_UX);
    /* map the PCI bus */
    map_region(0xa0000000, 0xa0000000, 0x40000000, TLB_SR | TLB_SW | TLB_UR | TLB_UW | TLB_G | TLB_I );
    /* PCI control registers and onboard devices */
    map_region(0xe0000000, 0xe0000000, 0x10000000, TLB_SR | TLB_SW | TLB_UR | TLB_UW | TLB_G | TLB_I);

    D(bug("[KRN] TLB status: %d used, %d free\n", 64 - tlb_free, tlb_free));
    
    /* flush TLB shadow regs */
    asm volatile("isync;");
}
