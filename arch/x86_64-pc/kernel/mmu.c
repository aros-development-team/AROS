#include <asm/cpu.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "apic.h"

#define DMMU(x)

/*
    The MMU pages and directories. They are stored at fixed location and may be either reused in the 
    64-bit kernel, or replaced by it. Four PDE directories (PDE2M structures) are enough to map whole
    4GB address space.
*/
struct PML4E PML4[512] __attribute__((used,aligned(4096)));
static struct PDPE PDP[512] __attribute__((used,aligned(4096)));
static struct PDE2M PDE[4][512] __attribute__((used,aligned(4096)));

void core_SetupMMU(struct KernBootPrivate *__KernBootPrivate)
{
    IPTR        _APICBase;
    UBYTE       _APICID;
    int i;
    struct PDE2M *pdes[] = { &PDE[0][0], &PDE[1][0], &PDE[2][0], &PDE[3][0] };

    _APICBase = __KernBootPrivate->kbp_APIC_Drivers[__KernBootPrivate->kbp_APIC_DriverID]->getbase();
    _APICID   = __KernBootPrivate->kbp_APIC_Drivers[__KernBootPrivate->kbp_APIC_DriverID]->getid(_APICBase);

    if (_APICID == __KernBootPrivate->kbp_APIC_BSPID)
    {
        bug("[Kernel] core_SetupMMU[%d]: Re-creating the MMU pages for first 4GB area\n", _APICID);

        /* PML4 Entry - we need only the first out of 16 entries */
        PML4[0].p  = 1; /* present */
        PML4[0].rw = 1; /* read/write */
        PML4[0].us = 1; /* accessible for user */
        PML4[0].pwt= 0; /* write-through cache */
        PML4[0].pcd= 0; /* cache enabled */
        PML4[0].a  = 0; /* not yet accessed */
        PML4[0].mbz= 0; /* must be zero */
        PML4[0].base_low = (unsigned long)PDP >> 12;
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
            PDP[i].base_low = (unsigned long)pdes[i] >> 12;

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
    }

    bug("[Kernel] core_SetupMMU[%d]: Registering New PML4\n", _APICID);

    wrcr(cr3, &PML4);

    bug("[Kernel] core_SetupMMU[%d]: PML4 @ %p\n", _APICID, &PML4);
}

static struct PTE Pages4K[32][512] __attribute__((used,aligned(4096)));
static int used_page;

void core_ProtPage(intptr_t addr, char p, char rw, char us)
{
    struct PML4E *pml4 = (struct PML4E *)rdcr(cr3);
    struct PDPE *pdpe  = (struct PDPE *)((unsigned long)pml4[(addr >> 39) & 0x1ff].base_low << 12);
    struct PDE4K *pde  = (struct PDE4K *)((unsigned long)pdpe[(addr >> 30) & 0x1ff].base_low << 12);
    struct PTE *pte;

    DMMU(bug("[Kernel] Marking page 0x%p as read-only\n", addr));

    if (pde[(addr >> 21) & 0x1ff].ps)
    {
        struct PDE2M *pde2 = (struct PDE2M *)pde;

        pte = Pages4K[used_page++];

        /* work on local copy of the affected PDE */
        struct PDE4K tmp_pde = pde[(addr >> 21) & 0x1ff]; 

        intptr_t base = pde2[(addr >> 21) & 0x1ff].base_low << 13;
        int i;

        bug("[Kernel] The page for address 0x%p was a big one. Splitting it into 4K pages\n", addr);
        bug("[Kernel] Base=0x%p, pte=0x%p\n", base, pte);

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
            
    pte = (struct PTE *)((unsigned long)pde[(addr >> 21) & 0x1ff].base_low << 12);

    pte[(addr >> 12) & 0x1ff].rw = rw ? 1:0;
    pte[(addr >> 12) & 0x1ff].us = us ? 1:0;
    pte[(addr >> 12) & 0x1ff].p = p ? 1:0;
    asm volatile ("invlpg (%0)"::"r"(addr));   
}

void core_ProtKernelArea(intptr_t addr, intptr_t length, char p, char rw, char us)
{
    bug("[Kernel] Protecting area 0x%p - 0x%p\n", addr, addr + length - 1);

    while (length > 0)
    {
        core_ProtPage(addr, p, rw, us);
        addr += 4096;
        length -= 4096;
    }
}
