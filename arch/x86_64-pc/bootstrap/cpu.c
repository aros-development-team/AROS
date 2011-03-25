#include <aros/kernel.h>

#include "bootstrap.h"
#include "cpu.h"
#include "support.h"

static struct
{
    struct segment_desc seg0;      /* seg 0x00 */
    struct segment_desc super_cs;  /* seg 0x08 */
    struct segment_desc super_ds;  /* seg 0x10 */
} GDT __attribute__((used,aligned(128),section(".bss.aros.tables")));

/*
    The MMU pages and directories. They are stored at fixed location and may be either reused in the 
    64-bit kernel, or replaced by it. Four PDE directories (PDE2M structures) are enough to map whole
    4GB address space.
*/
static struct PML4E PML4[512] __attribute__((used,aligned(4096),section(".bss.aros.tables")));
static struct PDPE PDP[512] __attribute__((used,aligned(4096),section(".bss.aros.tables")));
static struct PDE2M PDE[4][512] __attribute__((used,aligned(4096),section(".bss.aros.tables")));

static struct
{
    void *off;
    unsigned short seg;
} __attribute__((packed)) KernelTarget = { (void*)KERNEL_TARGET_ADDRESS, SEG_SUPER_CS };

/*
    32-bit pseudo-registers used to load the Global and Interrupt Descriptor Tables. They are used only once.
*/
const struct
{
    unsigned short size __attribute__((packed));
    unsigned int base __attribute__((packed));
} 
GDT_sel = {sizeof(GDT)-1, (unsigned int)&GDT};

/*
    Setting descriptor tables up. It is perhaps not wise to embed it into the function. Most likely the more
    effective solution would be to make a local copy of new descriptor table instead. But the code is more clear
    and takes effectively *no place* since it is loaded at bootup and removed later.
*/
static void setup_tables()
{
    kprintf("[BOOT] Setting up descriptor tables.\n");

    /* Supervisor segments */
    GDT.super_cs.type=0x1a;	/* code segment */
    GDT.super_cs.dpl=0;		/* supervisor level */
    GDT.super_cs.p=1;		/* present */
    GDT.super_cs.l=1;		/* long (64-bit) one */
    GDT.super_cs.d=0;		/* must be zero */
    GDT.super_cs.limit_low=0xffff;
    GDT.super_cs.limit_high=0xf;
    GDT.super_cs.g=1;
    GDT.super_cs.base_low=0;
    GDT.super_cs.base_mid=0;
    GDT.super_cs.base_high=0;
    
    GDT.super_ds.type=0x12;	/* data segment */
    GDT.super_ds.dpl=0;		/* supervisor level */
    GDT.super_ds.p=1;		/* present */
    GDT.super_ds.limit_low=0xffff;
    GDT.super_ds.limit_high=0xf;
    GDT.super_ds.g=1;
    GDT.super_ds.d=1;   
    GDT.super_ds.base_low=0;
    GDT.super_ds.base_mid=0;
    GDT.super_ds.base_high=0;
}

/*
    The 64-bit long mode may be activated only, when MMU paging is enabled. Therefore the basic
    MMU tables have to be prepared first. This routine uses 6 tables (2048 + 5 entries) in order
    to map the first 4GB of address space as user-accessible executable RW area.
    
    This mapping may be changed later by the 64-bit kernel, in order to provide separate address spaces,
    protect kernel from being overwritten and so on and so forth.
*/
void setup_mmu(void *kick_base)
{
    int i;
    struct PDE2M *pdes[] = { &PDE[0], &PDE[1], &PDE[2], &PDE[3] };

    KernelTarget.off = kick_base;
    setup_tables();

    kprintf("[BOOT] Mapping first 4G area with MMU\n");

    /* PML4 Entry - we need only the first out of 16 entries */
    PML4[0].p  = 1; /* present */
    PML4[0].rw = 1; /* read/write */
    PML4[0].us = 1; /* accessible for user */
    PML4[0].pwt= 0; /* write-through cache */
    PML4[0].pcd= 0; /* cache enabled */
    PML4[0].a  = 0; /* not yet accessed */
    PML4[0].mbz= 0; /* must be zero */
    PML4[0].base_low = (unsigned int)PDP >> 12;
    PML4[0].avl= 0;
    PML4[0].nx = 0;
    PML4[0].avail = 0;
    PML4[0].base_high = 0;

    /*
        PDP Entries. There are four of them used in order to define 2048 pages of 2MB each.
     */
    for (i=0; i < 4; i++)
    {
        int j;
	
        /* Set the PDP entry up and point to the PDE table */
        PDP[i].p  = 1;
        PDP[i].rw = 1;
        PDP[i].us = 1;
        PDP[i].pwt= 0;
        PDP[i].pcd= 0;
        PDP[i].a  = 0;
        PDP[i].mbz= 0;
        PDP[i].base_low = (unsigned int)pdes[i] >> 12;

        PDP[i].nx = 0;
        PDP[i].avail = 0;
        PDP[i].base_high = 0;

        for (j=0; j < 512; j++)
        {
            /* Set PDE entries - use 2MB memory pages, with full supervisor and user access */
	    
            struct PDE2M *PDE = pdes[i];
            PDE[j].p  = 1;
            PDE[j].rw = 1;
            PDE[j].us = 1;
            PDE[j].pwt= 0;  // 1
            PDE[j].pcd= 0;  // 1
            PDE[j].a  = 0;
            PDE[j].d  = 0;
            PDE[j].g  = 0;
            PDE[j].pat= 0;
            PDE[j].ps = 1;
            PDE[j].base_low = ((i << 30) + (j << 21)) >> 13;
            
            PDE[j].avail = 0;
            PDE[j].nx = 0;
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
static int leave_32bit_mode()
{
    unsigned int v1 = 0, v2 = 0, v3 = 0, v4 = 0;
    int retval = 0;
    asm volatile ("lgdt %0"::"m"(GDT_sel));

    cpuid(0x80000000, v1, v2, v3, v4);
    if (v1 > 0x80000000)
    { 
        cpuid(0x80000001, v1, v2, v3, v4);
        if (v4 & (1 << 29))
        {
            /* Enable PAE */
            wrcr(cr4, _CR4_PAE | _CR4_PGE);
            
            /* enable pages */
            wrcr(cr3, &PML4);
        
            /* enable long mode */
            rdmsr(EFER, &v1, &v2);
            v1 |= _EFER_LME;
            wrmsr(EFER, v1, v2);
        
            /* enable paging and activate long mode */
            wrcr(cr0, _CR0_PG | _CR0_PE);
            retval = 1;
        }
    }
    
    return retval;
}

/* Run the kickstart */
void kick(struct TagItem64 *km)
{
    /*
     * And do a necessary long jump into the 64-bit kernel
     */
    kprintf("[BOOT] Leaving 32-bit environment."
        " LJMP $%x,$%p\n\n", SEG_SUPER_CS, KernelTarget.off);

    if (leave_32bit_mode())
        asm volatile("ljmp *%0"::"m"(KernelTarget),"D"(&km));

    kprintf("Your processor is not x86-64 compatible\n");
}
