/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __KERNEL_NOLIBBASE__

#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <asm/cpu.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"


#define ROMSIZE2MB              (1 << 21)
#define ROMSIZE1MB              (1 << 20)
#define ROMSIZE512              (1 << 19)
#define ROMSIZE256              (1 << 18)

#define D(x)

static BOOL AMiGAROM_MatchWords(APTR roma, APTR romb)
{
    ULONG *roma_ptr = (ULONG *)roma;
    ULONG *romb_ptr = (ULONG *)romb;

    if ((roma_ptr[0] == romb_ptr[0]) &&
        (roma_ptr[1] == romb_ptr[1]) &&
        (roma_ptr[2] == romb_ptr[2]) &&
        (roma_ptr[3] == romb_ptr[3]))
        return TRUE;
    return FALSE;
}

static BOOL AMiGAROM_IsValid(APTR romimg)
{
    /* TODO: Check for a valid ROM header */
    return TRUE;
}

static int AMiGAROMSupport_Init(struct KernelBase *KernelBase)
{
    struct PlatformData *pd = KernelBase->kb_PlatformData;
    ULONG romsize, imgcnt;

    D(bug("[Kernel:Am68k] %s: platformdata @ 0x%p\n", __func__, pd);)

    if (ReadGayle() &&
        AMiGAROM_IsValid((APTR)0xA80000) &&
        !AMiGAROM_MatchWords((APTR)0xA80000, (APTR)0xF80000))
    {
        bug("ROMInfo: 2MiB ROM detected\n");
        romsize = ROMSIZE2MB;
        imgcnt = 4;
    }
    else if (AMiGAROM_IsValid((APTR)0xE00000) &&
        !AMiGAROM_MatchWords((APTR)0xE00000, (APTR)0xF80000))
    {
        bug("ROMInfo: 1MiB ROM detected\n");
        romsize = ROMSIZE1MB;
        imgcnt = 2;
    }
    else
    {
        bug("ROMInfo: 512KiB ROM detected\n");
        romsize = ROMSIZE512;
        imgcnt = 2;
    }

    if (pd->mmu_type)
    {
        void *(*getphysaddr)(void *virtaddr) = NULL;
        ULONG rommappings[(imgcnt << 1)];
        int i = 0;

        D(bug("[Kernel:Am68k] %s: mmutype = #%d\n", __func__, pd->mmu_type);)

        /* if there is an mmu present, detect if the roms have been remapped */
        if (pd->mmu_type == MMU040)
            getphysaddr = getphysaddr_mmu040;
        if (pd->mmu_type == MMU030)
            getphysaddr = getphysaddr_mmu030;

        if (getphysaddr)
        {
            APTR physaddr;

            /* System ROM */
            physaddr = getphysaddr((APTR)0xF80000);
            if (physaddr != (APTR)0xF80000)
            {
                D(bug("[Kernel:Am68k] %s: mmutype = %d\n", __func__, pd->mmu_type);)

                rommappings[i] = 0xF80000;
                rommappings[i + 1] = (ULONG)physaddr;
                i += 2;
            }
            if (romsize > ROMSIZE512)
            {
                /* Extended ROM */
                physaddr = getphysaddr((APTR)0xE00000);
                if (physaddr != (APTR)0xE00000)
                {
                    rommappings[i] = 0xE00000;
                    rommappings[i + 1] = (ULONG)physaddr;
                    i += 2;
                }
                if (romsize > ROMSIZE1MB)
                {
                    /* Workbench ROM */
                    physaddr = getphysaddr((APTR)0xB00000);
                    if (physaddr != (APTR)0xB00000)
                    {
                        rommappings[i] = 0xB00000;
                        rommappings[i + 1] = (ULONG)physaddr;
                        i += 2;
                    }
                    physaddr = getphysaddr((APTR)0xA80000);
                    if (physaddr != (APTR)0xA80000)
                    {
                        rommappings[i] = 0xA80000;
                        rommappings[i + 1] = (ULONG)physaddr;
                        i += 2;
                    }
                }
            }
        }
        if (i > 0)
        {
            pd->romimg = AllocMem(sizeof(ULONG) * (i + 2), MEMF_ANY);
            CopyMem(rommappings, pd->romimg, sizeof(ULONG) * i);
            pd->romsize = romsize / imgcnt;
        }
    }

    APTR physaddr;
    bug("ROMInfo: ROM region(s)..\n");
    switch (romsize)
    {
        case ROMSIZE2MB:
            {
                physaddr = KrnVirtualToPhysical((APTR)0xA80000);
                bug("ROMInfo:     0x%08x - 0x%08x", 0xA80000, 0xA80000 + (romsize / imgcnt) - 1);
                if (physaddr != (APTR)0xA80000)
                    bug("@  0x%08x - 0x%08x", physaddr, (IPTR)physaddr + (romsize / imgcnt) - 1);
                bug("\n");
                physaddr = KrnVirtualToPhysical((APTR)0xB00000);
                bug("ROMInfo:     0x%08x - 0x%08x", 0xB00000, 0xB00000 + (romsize / imgcnt) - 1);
                if (physaddr != (APTR)0xB00000)
                    bug("@  0x%08x - 0x%08x", physaddr, (IPTR)physaddr + (romsize / imgcnt) - 1);
                bug("\n");
            }
        case ROMSIZE1MB:
            {
                physaddr = KrnVirtualToPhysical((APTR)0xE00000);
                bug("ROMInfo:     0x%08x - 0x%08x", 0xE00000, 0xE00000 + (romsize / imgcnt) - 1);
                if (physaddr != (APTR)0xE00000)
                    bug("@  0x%08x - 0x%08x", physaddr, (IPTR)physaddr + (romsize / imgcnt) - 1);
                bug("\n");
            }
        case ROMSIZE512:
            {
                physaddr = KrnVirtualToPhysical((APTR)0xF80000);
                bug("ROMInfo:     0x%08x - 0x%08x", 0xF80000, 0xF80000 + (romsize / imgcnt) - 1);
                if (physaddr != (APTR)0xF80000)
                    bug("@  0x%08x - 0x%08x", physaddr, (IPTR)physaddr + (romsize / imgcnt) - 1);
                bug("\n");
            }
    }
    return TRUE;
}

ADD2INITLIB(AMiGAROMSupport_Init, 120)
