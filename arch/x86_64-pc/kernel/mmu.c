#include <asm/cpu.h>

#include "kernel_intern.h"

/*
    The MMU pages and directories. They are stored at fixed location and may be either reused in the 
    64-bit kernel, or replaced by it. Four PDE directories (PDE2M structures) are enough to map whole
    4GB address space.
*/
static struct PML4E PML4[512] __attribute__((used,aligned(4096)));
static struct PDPE PDP[512] __attribute__((used,aligned(4096)));
static struct PDE2M PDE[4][512] __attribute__((used,aligned(4096)));

void core_SetupMMU()
{
    int i;
    struct PDE2M *pdes[] = { &PDE[0], &PDE[1], &PDE[2], &PDE[3] };
    
    rkprintf("[Kernel] Re-creating the MMU pages for first 4GB area\n");

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
    
    wrcr(cr3, &PML4);
    rkprintf("[Kernel] PML4 @ %012p\n", &PML4);
}

static struct PTE Pages4K[32][512] __attribute__((used,aligned(4096)));
static int used_page;

void core_ProtPage(intptr_t addr, char p, char rw, char us)
{
    struct PML4E *pml4 = rdcr(cr3);
    struct PDPE *pdpe = pml4[(addr >> 39) & 0x1ff].base_low << 12;
    struct PDE4K *pde = pdpe[(addr >> 30) & 0x1ff].base_low << 12;
    
    rkprintf("[Kernel] Marking page %012p as read-only\n",addr);
    
    if (pde[(addr >> 21) & 0x1ff].ps)
    {
        struct PTE *pte = Pages4K[used_page++];
        struct PDE2M *pde2 = (struct PDE2M *)pde;
        
        /* work on local copy of the affected PDE */
        struct PDE4K tmp_pde = pde[(addr >> 21) & 0x1ff]; 
        
        intptr_t base = pde2[(addr >> 21) & 0x1ff].base_low << 13;
        int i;
        
        rkprintf("[Kernel] The page for address %012p was a big one. Splitting it into 4K pages\n",
                 addr);
        rkprintf("[Kernel] Base=%012p, pte=%012p\n", base, pte);
        
        for (i = 0; i < 512; i++)
        {
            pte[i].p = 1;
            pte[i].rw = pde2[(addr >> 21) & 0x1ff].rw;
            pte[i].us = pde2[(addr >> 21) & 0x1ff].us;
            pte[i].pwt = pde2[(addr >> 21) & 0x1ff].pwt;
            pte[i].pcd = pde2[(addr >> 21) & 0x1ff].pcd;
            pte[i].base_low = base >> 12;
            base += 4096;
        }
        
        tmp_pde.ps = 0;
        tmp_pde.base_low = ((intptr_t)pte) >> 12;
        
        pde[(addr >> 21) & 0x1ff] = tmp_pde;
    }
            
    struct PTE *pte = pde[(addr >> 21) & 0x1ff].base_low << 12;
    pte[(addr >> 12) & 0x1ff].rw = rw ? 1:0;
    pte[(addr >> 12) & 0x1ff].us = us ? 1:0;
    pte[(addr >> 12) & 0x1ff].p = p ? 1:0;
    asm volatile ("invlpg (%0)"::"r"(addr));   
}

void core_ProtKernelArea(intptr_t addr, intptr_t length, char p, char rw, char us)
{
    rkprintf("[Kernel] Protecting area %012p-%012p\n", addr, addr + length - 1);

    while (length > 0)
    {
        core_ProtPage(addr, p, rw, us);
        addr += 4096;
        length -= 4096;
    }
}
