/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <asm/x86_64/cpu.h>

#include "bootstrap.h"
#include "support.h"

/* Segment registers */
#define SEG_SUPER_CS    0x08
#define SEG_SUPER_DS    0x10
#define SEG_USER_CS32   0x18
#define SEG_USER_CS64   0x28
#define SEG_USER_DS     0x20
#define SEG_TSS         0x30

/* Global descriptor table */
static struct
{
    struct segment_desc seg0;      /* seg 0x00 */
    struct segment_desc super_cs;  /* seg 0x08 */
    struct segment_desc super_ds;  /* seg 0x10 */
} GDT __attribute__((used,aligned(128),section(".bss.aros.tables")));

/* Data used to load GDTR */
const struct segment_selector GDT_sel =
{
    sizeof(GDT)-1,
    (unsigned long)&GDT
};

/* Far jump detination specification (address and segment selector */
static struct
{
    void *off;
    unsigned short seg;
} __attribute__((packed)) KernelTarget =
{
    (void*)KERNEL_TARGET_ADDRESS,
    SEG_SUPER_CS
};

/*
 * The MMU pages and directories. They are stored at fixed location and may be either reused in the 
 * 64-bit kernel, or replaced by it. Four PDE directories (PDE2M structures) are enough to map whole
 * 4GB address space.
 */
static struct PML4E PML4[512]   __attribute__((used,aligned(4096),section(".bss.aros.tables")));
static struct PDPE  PDP[512]    __attribute__((used,aligned(4096),section(".bss.aros.tables")));
static struct PDE2M PDE[4][512] __attribute__((used,aligned(4096),section(".bss.aros.tables")));

/*
 * The 64-bit long mode may be activated only, when MMU paging is enabled. Therefore the basic
 * MMU tables have to be prepared first. This routine uses 6 tables (2048 + 5 entries) in order
 * to map the first 4GB of address space as user-accessible executable RW area.
 *
 * This mapping may be changed later by the 64-bit kernel, in order to provide separate address spaces,
 * protect kernel from being overwritten and so on and so forth.
 *
 * To simplify things down we will use 2MB memory page size. In this mode the address is broken up into 4 fields:
 * - Bits 63–48 sign extension of bit 47 as required for canonical address forms.
 * - Bits 47–39 index into the 512-entry page-map level-4 table. 
 * - Bits 38–30 index into the 512-entry page-directory-pointer table.
 * - Bits 29–21 index into the 512-entry page-directory table.
 * - Bits 20–0  byte offset into the physical page.
 * Let's remember that our topmost address is  0xFFFFF000, as specified by GDT.
*/
void setup_mmu(void)
{
    int i;
    struct PDE2M *pdes[] = { &PDE[0][0], &PDE[1][0], &PDE[2][0], &PDE[3][0] };

    D(kprintf("[BOOT] Setting up MMU, kickstart base 0x%p\n", kick_base));
    D(kprintf("[BOOT] cr0: 0x%p cr3: 0x%p cr4: 0x%p\n", rdcr(cr0), rdcr(cr3), rdcr(cr4)));

    D(kprintf("[BOOT] Setting up descriptor tables.\n"));

    /* Supervisor code segment */
    GDT.super_cs.type	    = 0x1a;	/* code, non-conforming, readable	*/
    GDT.super_cs.dpl	    = 0;	/* supervisor level (ring 0)		*/
    GDT.super_cs.p	    = 1;	/* present				*/
    GDT.super_cs.l	    = 1;	/* long mode enabled			*/
    GDT.super_cs.d	    = 0;	/* must be zero for long mode		*/
    GDT.super_cs.limit_low  = 0xffff;	/* Limit is actually 0xFFFFF000		*/
    GDT.super_cs.limit_high = 0xf;
    GDT.super_cs.g	    = 1;	/* Limit is in 4K pages			*/
    GDT.super_cs.base_low   = 0;	/* Segment starts at zero address	*/
    GDT.super_cs.base_mid   = 0;
    GDT.super_cs.base_high  = 0;

    /* Supervisor data segment. Actually ignored in long mode. */
    GDT.super_ds.type	    = 0x12;	/* data, expand up, writable		*/
    GDT.super_ds.dpl	    = 0;	/* supervisor level			*/
    GDT.super_ds.p	    = 1;	/* present				*/
    GDT.super_ds.limit_low  = 0xffff;	/* Limit = 0xFFFFF000			*/
    GDT.super_ds.limit_high = 0xf;
    GDT.super_ds.g	    = 1;	/* 4K granularity			*/
    GDT.super_ds.d	    = 1;	/* 32-bit operands			*/
    GDT.super_ds.base_low   = 0;	/* Start at zero address		*/
    GDT.super_ds.base_mid   = 0;
    GDT.super_ds.base_high  = 0;

    D(kprintf("[BOOT] Mapping first 4G area with MMU\n"));
    D(kprintf("[BOOT] PML4 0x%p, PDP 0x%p, PDE 0x%p\n", PML4, PDP, PDE));

    /*
     * Page map level 4 Entry.
     * Since we actually use only 32-bit addresses, we need only one entry
     * number zero (bits 47-39 of our address are zeroes).
     */
    PML4[0].p	      = 1;			 /* present in physical RAM				*/
    PML4[0].rw	      = 1;			 /* read/write access					*/
    PML4[0].us	      = 1;			 /* accessible on user level				*/
    PML4[0].pwt	      = 0;		         /* write-through cache mode				*/
    PML4[0].pcd	      = 0;			 /* caching enabled					*/
    PML4[0].a	      = 0;			 /* clear access bit (just in case)			*/
    PML4[0].mbz	      = 0;			 /* reserved, must be zero				*/
    PML4[0].avl	      = 0;			 /* user-defined flags, clear them			*/
    PML4[0].base_low  = (unsigned int)PDP >> 12; /* Base address of directory pointer table to use	*/
    PML4[0].nx	      = 0;			 /* code execution allowed				*/
    PML4[0].avail     = 0;			 /* more user-defined flags				*/
    PML4[0].base_high = 0;

    /*
     * Page directory pointer entries.
     * Our address contains usable bits 30 and 31, so there are four of them.
     */
    for (i=0; i < 4; i++)
    {
        int j;

	D(kprintf("[BOOT] PDE[%u] 0x%p\n", i, pdes[i]));

        /*
         * Set the PDP entry up and point to the PDE table.
         * Field meanings are analogous to PML4, just 'base' points to page directory tables
         */
        PDP[i].p 	 = 1;
        PDP[i].rw	 = 1;
        PDP[i].us	 = 1;
        PDP[i].pwt	 = 0;
        PDP[i].pcd	 = 0;
        PDP[i].a	 = 0;
        PDP[i].mbz	 = 0;
        PDP[i].base_low  = (unsigned int)pdes[i] >> 12;
        PDP[i].nx	 = 0;
        PDP[i].avail	 = 0;
        PDP[i].base_high = 0;

        for (j=0; j < 512; j++)
        {
            /* Build a complete PDE set (512 entries) for every PDP entry */
            struct PDE2M *PDE = pdes[i];

            PDE[j].p	     = 1;
            PDE[j].rw	     = 1;
            PDE[j].us	     = 1;
            PDE[j].pwt	     = 0;
            PDE[j].pcd	     = 0;
            PDE[j].a	     = 0;
            PDE[j].d	     = 0;	/* Clear write tracking bit							   */
            PDE[j].g	     = 0;	/* Page is global								   */
            PDE[j].pat	     = 0;	/* Most significant PAT bit							   */
            PDE[j].ps	     = 1;	/* It's PDE (not PTE) and page size will be 2MB (after we enable PAE)		   */
            PDE[j].base_low  = ((i << 30) + (j << 21)) >> 13;	/* Base address of the physical page. This is 1:1 mapping. */
            PDE[j].avail     = 0;
            PDE[j].nx	     = 0;
            PDE[j].base_high = 0;
        }
    }

    tag->ti_Tag = KRN_GDT;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long)&GDT;
    tag++;

    tag->ti_Tag = KRN_PL4;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long)&PML4;
    tag++;
}

/*
 * This tiny procedure sets the complete 64-bit environment up - it loads the descriptors, 
 * enables 64-bit mode, loads MMU tables and through paging it activates the 64-bit long mode.
 * 
 * After that it is perfectly safe to jump into the pure 64-bit kernel.
 */
void kick(void *kick_base, struct TagItem64 *km)
{
    unsigned int v1 = 0, v2 = 0, v3 = 0, v4 = 0;

    cpuid(0x80000000, v1, v2, v3, v4);
    if (v1 > 0x80000000)
    { 
        cpuid(0x80000001, v1, v2, v3, v4);
        if (v4 & (1 << 29))
        {
            D(kprintf("[BOOT] x86-64 CPU ok\n"));

	    KernelTarget.off = kick_base;

	    asm volatile ("lgdt %0"::"m"(GDT_sel));
	    D(kprintf("[BOOT] GDTR loaded\n"));

            /* Enable PAE */
            wrcr(cr4, _CR4_PAE | _CR4_PGE);
            D(kprintf("[BOOT] PAE is on\n"));
            
            /* enable pages */
            wrcr(cr3, &PML4);
            D(kprintf("[BOOT] cr3 loaded\n"));

            /* enable long mode */
            rdmsr(EFER, &v1, &v2);
            v1 |= _EFER_LME;
            wrmsr(EFER, v1, v2);
            D(kprintf("[BOOT] Long mode is on\n"));

            /* enable paging and activate long mode */
            wrcr(cr0, _CR0_PG | _CR0_PE);

	    kprintf("[BOOT] Leaving 32-bit environment. LJMP $%x,$%p\n\n", SEG_SUPER_CS, KernelTarget.off);
	    asm volatile("ljmp *%0"::"m"(KernelTarget),"D"(km),"S"(AROS_BOOT_MAGIC));
        }
    }

    kprintf("Your processor is not x86-64 compatible\n");
}
