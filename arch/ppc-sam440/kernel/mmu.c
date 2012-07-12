#include <aros/debug.h>
#include <asm/amcc440.h>
#include <asm/io.h>
#include <aros/kernel.h>

#include "kernel_intern.h"

struct tlb_info {
        uint32_t bitmap[2];
        int      free;
};

/* Alloc TLB in the bitmap. Returns -1 if the allocation cannot be done */
static int alloc_tlb(struct tlb_info *info)
{
    /* It should be done in locked state only! */
    
    int bit = __builtin_clz(info->bitmap[0]);
    if (bit == 32)
    {
        bit += __builtin_clz(info->bitmap[1]);
    }
    else
    {
        info->bitmap[0] &= ~(0x80000000 >> bit);
    }
    if (bit == 64)
    {
        return -1;
    }
    else
    {
        info->bitmap[1] &= ~(0x80000000 >> (bit-32));
    }
    info->free--;
    return bit;
}

static void free_tlb(struct tlb_info *info, int entry)
{
    if (entry >=0 && entry < 32)
    {
        asm volatile("tlbwe %0,%1,0;" ::"r"(0), "r"(entry));
        if (info->bitmap[0] & (0x80000000 >> entry))
        {
            D(bug("[KRN] Freeing already free TLB!!!\n"));
        }
        else
        {
            info->bitmap[0] |= (0x80000000 >> entry);
            info->free++;
        }
    }
    else if (entry < 64)
    {
        asm volatile("tlbwe %0,%1,0;" ::"r"(0), "r"(entry));
        entry -= 32;
        if (info->bitmap[1] & (0x80000000 >> entry))
        {
            D(bug("[KRN] Freeing already free TLB!!!\n"));
        }
        else
        {
            info->bitmap[1] |= (0x80000000 >> entry);
            info->free++;
        }
    }
    else
    {
        D(bug("[KRN] Wrong TLB\n"));
        return;
    }
}

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

void map_region(struct tlb_info *info, uint8_t extra, uintptr_t physbase, uintptr_t virtbase, uintptr_t length, uint32_t prot)
{
    long tlb_temp = info->free;
    
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
        tlb = alloc_tlb(info);
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
                     ::"r"(virtbase | allowable_pages[i].code | TLB_V), "r"(physbase | extra), "r"(prot), "r"(tlb));
        //D(bug("%08x %08x %08x ", virtbase | allowable_pages[i].code | 0x200, physbase, prot));
        
        length -= allowable_pages[i].mask + 1;
        physbase += allowable_pages[i].mask + 1;
        virtbase += allowable_pages[i].mask + 1;
        
    }
    tlb_temp -= info->free;
    D(bug("%2d TLB%s\n", tlb_temp, tlb_temp > 1 ? "s":""));
}

static void free_remaining(struct tlb_info *info)
{
    int tlb;

    if (info->free == 0)
        return;

    tlb = alloc_tlb(info);
    D(bug("[KRN] TLB%02x: Clear\n", tlb));
    free_remaining(info);
    free_tlb(info, tlb);
}

#if DEBUG
struct tlb { int tlb; uint32_t reg[3]; };

static void tlb_dump_entry(const struct tlb *tlb)
{
    uint32_t tlb_0, tlb_1, tlb_2;
    uint32_t phys, virt, size;

    tlb_0 = tlb->reg[0];
    tlb_1 = tlb->reg[1];
    tlb_2 = tlb->reg[2];

    if (!(tlb_0 & TLB_V))
        return;

    size = 1024 << (2 * ((tlb_0 >> 4) & 0xf));
    virt = tlb_0 & ~((1 << 10)-1);
    phys = tlb_1 & ~((1 << 10)-1);

    D(bug("[KRN] TLB%02x: ",tlb->tlb));
    D(bug("%c%c%c%c%c%c%c%c%c%c%c ",
          (tlb_2 & TLB_W) ? 'W' : '-',
          (tlb_2 & TLB_I) ? 'I' : '-',
          (tlb_2 & TLB_M) ? 'M' : '-',
          (tlb_2 & TLB_G) ? 'G' : '-',
          (tlb_2 & TLB_E) ? 'E' : '-',
          (tlb_2 & TLB_UR) ? 'r' : '-',
          (tlb_2 & TLB_UW) ? 'w' : '-',
          (tlb_2 & TLB_UX) ? 'x' : '-',
          (tlb_2 & TLB_SR) ? 'r' : '-',
          (tlb_2 & TLB_SW) ? 'w' : '-',
          (tlb_2 & TLB_SX) ? 'x' : '-'));
    D(bug("%08x - %08x : %08x: 0:%08x 1:%08x 2:%08x\n",
                virt, virt + size - 1, phys,
                tlb_0, tlb_1, tlb_2));
}

static int tlbcmp(const void *a, const void *b)
{
    const struct tlb *ta =a , *tb = b;

    if (ta->reg[0] < tb->reg[0])
        return -1;
    if (ta->reg[0] == tb->reg[0])
        return 0;
    return 1;
}

#include <stdlib.h>

static void tlb_dump(void)
{
    int tlb;
    struct tlb tlbs[64];
    D(static int some_bss);
    D(static int some_data=0xdeadcafe);

    D(bug("[KRN] Executing at %p, stack at %p, bss at %p, data at %p\n", __builtin_return_address(0), __builtin_frame_address(0), &some_bss, &some_data));
    for (tlb = 0; tlb < 64; tlb++) {
        asm volatile("tlbre %0,%3,0; tlbre %1,%3,1; tlbre %2,%3,2"
                     :"=r" (tlbs[tlb].reg[0]),
                      "=r" (tlbs[tlb].reg[1]),
                      "=r" (tlbs[tlb].reg[2])
                     :"r"(tlb));
        tlbs[tlb].tlb = tlb;
    }

    qsort(tlbs, 64, sizeof(tlbs[0]), tlbcmp);

    for (tlb = 0; tlb < 64; tlb++)
        tlb_dump_entry(&tlbs[tlb]);
}
#endif

void mmu_init(struct TagItem *tags)
{
    uintptr_t krn_lowest  = krnGetTagData(KRN_KernelLowest,  0, tags);
    uintptr_t krn_highest = krnGetTagData(KRN_KernelHighest, 0, tags);
    uintptr_t krn_base    = krnGetTagData(KRN_KernelBase,    0, tags);
    struct tlb_info info = {
        .bitmap = { ~0, ~0 },
        .free = 64
    };

    uint32_t pvr = rdspr(PVR);

    D(bug("[KRN] MMU Init\n"));
    D(bug("[KRN] lowest = %p, base = %p, highest = %p\n", krn_lowest, krn_base, krn_highest));
    D(bug("[KRN] Kernel size: %dKB code, %dKB data\n", (krn_highest - krn_base)/1024, (krn_base - krn_lowest)/1024));

    D(tlb_dump());

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
    map_region(&info, 0x0, krn_base, 0xff000000 + krn_base, krn_highest - krn_base, TLB_SR | TLB_SX | TLB_SW | TLB_UR | TLB_UX);
    /* Now the data area for kernel. Make it read/write for both user and supervisor. No execution allowed */
    map_region(&info, 0x0, krn_lowest, 0xff000000 + krn_lowest, krn_base - krn_lowest, TLB_SR | TLB_SW | TLB_UR | TLB_UW);
    /* The low memory will be RW for the supervisor mode, RO from user */
    map_region(&info, 0x0, 0, 0xff000000, krn_lowest, TLB_SR | TLB_SW | TLB_UR);
    
    /* The regular RAM, make 1GB of it - amcc440 cannot do more. */
    map_region(&info, 0x0, krn_highest, krn_highest, 0x40000000 - krn_highest, TLB_SR | TLB_SW | TLB_UR | TLB_UW | TLB_SX | TLB_UX);

    if (krnIsPPC440(pvr)) {
        D(bug("[KRN] MMU: Configure for PPC440\n"));
        /* map some 440EP peripherials bus */
        map_region(&info, 0x0, 0x80000000, 0x80000000, 0x20000000, TLB_SR | TLB_SW | TLB_UR | TLB_UW | TLB_G | TLB_I );
        /* map the PCI bus */
        map_region(&info, 0x0, 0xa0000000, 0xa0000000, 0x40000000, TLB_SR | TLB_SW | TLB_UR | TLB_UW | TLB_G | TLB_I );
        /* PCI control registers and onboard devices */
        map_region(&info, 0x0, 0xe0000000, 0xe0000000, 0x10000000, TLB_SR | TLB_SW | TLB_UR | TLB_UW | TLB_G | TLB_I);
    } else if (krnIsPPC460(pvr)) {
        D(bug("[KRN] MMU: Configure for PPC460\n"));
        /* PCI Memory             0x80000000-0xa0000000 */
        map_region(&info, 0xc, 0x80000000, 0x80000000, 0x20000000, TLB_SR | TLB_SW | TLB_UR | TLB_UW | TLB_G | TLB_I );
        /* PCI IO/Control         0xe8000000-0xef000000 */
        map_region(&info, 0xc, 0x08000000, 0xe8000000, 0x07000000, TLB_SR | TLB_SW | TLB_UR | TLB_UW | TLB_G | TLB_I);
        /* USB                    0xef000000-0xef010000 */
        map_region(&info, 0x4, 0xbffd0000, 0xef000000, 0x00010000, TLB_SR | TLB_SW | TLB_UR | TLB_UW | TLB_G | TLB_I);
        /* UART, GPT, ZMII, EMAC  0xef600000-0xef610000 */
        map_region(&info, 0x4, 0xef600000, 0xef600000, 0x00010000, TLB_SR | TLB_SW | TLB_UR | TLB_UW | TLB_G | TLB_I);
    } else {
        bug("[KRN] MMU: Cannot configure - unknown PVR model 0x%08x\n", pvr);
        for(;;);
    }

    free_remaining(&info);
    D(tlb_dump());

    D(bug("[KRN] TLB status: %d used, %d free\n", 64 - info.free, info.free));
    
    /* flush TLB shadow regs */
    asm volatile("isync;");
}
