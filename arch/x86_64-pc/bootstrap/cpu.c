/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.
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
 * 64-bit kernel, or replaced by it.
 *
 * 512 PDP entries × 512 PDEs each = 262144 2MB pages = 512 GiB identity map.
 * EFI firmware (especially with ReBAR / large-VRAM GPUs) may place GOP framebuffers
 * far above 4 GiB. Keeping a wider early identity map avoids triple-fault resets
 * when bootconsole touches a high framebuffer before the kernel rebuilds MMU tables.
 */
#define BOOTSTRAP_PDP_COUNT 512

static struct PML4E PML4[512]                    __attribute__((used,aligned(4096),section(".bss.aros.tables")));
static struct PDPE  PDP[512]                     __attribute__((used,aligned(4096),section(".bss.aros.tables")));
static struct PDE2M PDE[BOOTSTRAP_PDP_COUNT][512] __attribute__((used,aligned(4096),section(".bss.aros.tables")));

/*
 * The 64-bit long mode may be activated only, when MMU paging is enabled. Therefore the basic
 * MMU tables have to be prepared first.
 *
 * This routine creates a 512 GiB identity map using 2MB pages so that the ljmp into the 64-bit
 * kernel succeeds even when GRUB2/EFI places kernel modules above 4 GiB.
 *
 * This mapping may be changed later by the 64-bit kernel, in order to provide separate address spaces,
 * protect kernel from being overwritten and so on and so forth.
 *
 * To simplify things down we will use 2MB memory page size. In this mode the address is broken up into 4 fields:
 * - Bits 63-48 sign extension of bit 47 as required for canonical address forms.
 * - Bits 47-39 index into the 512-entry page-map level-4 table.
 * - Bits 38-30 index into the 512-entry page-directory-pointer table.
 * - Bits 29-21 index into the 512-entry page-directory table.
 * - Bits 20-0  byte offset into the physical page.
 * Let's remember that our topmost address is  0xFFFFF000, as specified by GDT.
*/
void setup_mmu(void)
{
    int i;

    D(kprintf("[BOOT] Setting up MMU, kickstart base 0x%p\n", kick_base);)
    D(kprintf("[BOOT] cr0: 0x%p cr3: 0x%p cr4: 0x%p\n", rdcr(cr0), rdcr(cr3), rdcr(cr4));)

    D(kprintf("[BOOT] Setting up descriptor tables.\n");)

    /* Supervisor code segment */
    GDT.super_cs.type       = 0x1a;     /* code, non-conforming, readable       */
    GDT.super_cs.dpl        = 0;        /* supervisor level (ring 0)            */
    GDT.super_cs.p          = 1;        /* present                              */
    GDT.super_cs.l          = 1;        /* long mode enabled                    */
    GDT.super_cs.d          = 0;        /* must be zero for long mode           */
    GDT.super_cs.limit_low  = 0xffff;   /* Limit is actually 0xFFFFF000         */
    GDT.super_cs.limit_high = 0xf;
    GDT.super_cs.g          = 1;        /* Limit is in 4K pages                 */
    GDT.super_cs.base_low   = 0;        /* Segment starts at zero address       */
    GDT.super_cs.base_mid   = 0;
    GDT.super_cs.base_high  = 0;

    /* Supervisor data segment. Actually ignored in long mode. */
    GDT.super_ds.type       = 0x12;     /* data, expand up, writable            */
    GDT.super_ds.dpl        = 0;        /* supervisor level                     */
    GDT.super_ds.p          = 1;        /* present                              */
    GDT.super_ds.limit_low  = 0xffff;   /* Limit = 0xFFFFF000                   */
    GDT.super_ds.limit_high = 0xf;
    GDT.super_ds.g          = 1;        /* 4K granularity                       */
    GDT.super_ds.d          = 1;        /* 32-bit operands                      */
    GDT.super_ds.base_low   = 0;        /* Start at zero address                */
    GDT.super_ds.base_mid   = 0;
    GDT.super_ds.base_high  = 0;

    D(kprintf("[BOOT] Mapping first %dG area with MMU\n", BOOTSTRAP_PDP_COUNT);)
    D(kprintf("[BOOT] PML4 0x%p, PDP 0x%p, PDE 0x%p\n", PML4, PDP, PDE);)

    /*
     * Page map level 4 Entry.
     * Since we actually use only 32-bit addresses, we need only one entry
     * number zero (bits 47-39 of our address are zeroes).
     */
    PML4[0].p         = 1;                       /* present in physical RAM                             */
    PML4[0].rw        = 1;                       /* read/write access                                   */
    PML4[0].us        = 1;                       /* accessible on user level                            */
    PML4[0].pwt       = 0;                       /* write-through cache mode                            */
    PML4[0].pcd       = 0;                       /* caching enabled                                     */
    PML4[0].a         = 0;                       /* clear access bit (just in case)                     */
    PML4[0].mbz       = 0;                       /* reserved, must be zero                              */
    PML4[0].avl       = 0;                       /* user-defined flags, clear them                      */
    PML4[0].base_low  = (unsigned int)PDP >> 12; /* Base address of directory pointer table to use      */
    PML4[0].nx        = 0;                       /* code execution allowed                              */
    PML4[0].avail     = 0;                       /* more user-defined flags                             */
    PML4[0].base_high = 0;

    /*
     * Page directory pointer entries.
     * We map BOOTSTRAP_PDP_COUNT GiB (one PDP entry = 1 GiB with 512 × 2MB pages).
     */
    for (i = 0; i < BOOTSTRAP_PDP_COUNT; i++)
    {
        int j;
        struct PDE2M *pde = &PDE[i][0];

        D(if ((i & 15) == 0) kprintf("[BOOT] PDP[%u] PDE 0x%p\n", i, pde);)

        /*
         * Set the PDP entry up and point to the PDE table.
         * Bootstrap BSS is always below 4 GiB (loaded by GRUB2 in low memory),
         * so base_high for the PDE table pointer is always 0.
         */
        PDP[i].p         = 1;
        PDP[i].rw        = 1;
        PDP[i].us        = 1;
        PDP[i].pwt       = 0;
        PDP[i].pcd       = 0;
        PDP[i].a         = 0;
        PDP[i].mbz       = 0;
        PDP[i].base_low  = (unsigned int)pde >> 12;
        PDP[i].nx        = 0;
        PDP[i].avail     = 0;
        PDP[i].base_high = 0;

        for (j = 0; j < 512; j++)
        {
            /*
             * Build a complete PDE set (512 entries) for every PDP entry.
             * Use unsigned long long for the physical base address since
             * we're in 32-bit mode but mapping addresses above 4 GiB.
             */
            unsigned long long base = ((unsigned long long)i << 30) | ((unsigned long long)j << 21);

            pde[j].p         = 1;
            pde[j].rw        = 1;
            pde[j].us        = 1;
            pde[j].pwt       = 0;
            pde[j].pcd       = 0;
            pde[j].a         = 0;
            pde[j].d         = 0;
            pde[j].g         = 0;
            pde[j].pat       = 0;
            pde[j].ps        = 1;       /* 2MB page size                                                                   */
            pde[j].base_low  = (unsigned int)(base >> 13);  /* Physical page base address bits [31:13]                     */
            pde[j].avail     = 0;
            pde[j].nx        = 0;
            pde[j].base_high = (unsigned int)((base >> 32) & 0x000FFFFF); /* Physical page base address bits [51:32]       */
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
    unsigned int eax0, ebx0, ecx0, edx0;
    /* Get extended max leaf */
    cpuid2(0x80000000, 0, &eax0, &ebx0, &ecx0, &edx0);

    if (eax0 >= 0x80000001) {
        unsigned int eax1, ebx1, ecx1, edx1;
        /* Read LM bit from 0x80000001 EDX */
        cpuid2(0x80000001, 0, &eax1, &ebx1, &ecx1, &edx1);

        if (edx1 & (1u << 29)) {                /* edx1 == EDX (LM) */
            D(kprintf("[BOOT] 64-bit CPU detected\n");)
            KernelTarget.off = kick_base;
            asm volatile ("lgdt %0"::"m"(GDT_sel));
            D(kprintf("[BOOT] GDTR loaded\n");)
            /* Enable PAE */
            wrcr(cr4, _CR4_PAE | _CR4_PGE);
            D(kprintf("[BOOT] PAE enabled\n");)
            /* enable pages */
            wrcr(cr3, &PML4);
            D(kprintf("[BOOT] Page tables loaded\n");)
            /* enable long mode */
            {
                unsigned int efer_low, efer_high;
                rdmsr(EFER, &efer_low, &efer_high);
                efer_low |= _EFER_LME;
                wrmsr(EFER, efer_low, efer_high);
            }
            D(kprintf("[BOOT] Long mode enabled\n");)
            /* paging + protected -> long mode */
            wrcr(cr0, _CR0_PG | _CR0_PE);
            D(kprintf("[BOOT] Leaving 32-bit environment. LJMP $%x,$%p\n\n", SEG_SUPER_CS, KernelTarget.off);)
            asm volatile("ljmp *%0"::"m"(KernelTarget),"D"(km),"S"(AROS_BOOT_MAGIC));
            __builtin_unreachable();
        }
        /* Failure path: dump actual regs from 0x80000001 */
        kprintf("[BOOT] CPUID(0x80000001) <EAX 0x%08x, EBX 0x%08x, ECX 0x%08x, EDX 0x%08x>\n",
                eax1, ebx1, ecx1, edx1);
    } else {
        kprintf("[BOOT] CPUID(0x80000000) <EAX 0x%08x, EBX 0x%08x, ECX 0x%08x, EDX 0x%08x>\n",
                eax0, ebx0, ecx0, edx0);
    }
    kprintf("\nThis processor is not x86-64 compatible\n");
}
