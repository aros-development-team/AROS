/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
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

extern BYTE _rom_start;
extern BYTE _rom_end;
extern BYTE _ext_start;
extern BYTE _ext_end;

#define ROMSIZE2MB              (1 << 21)
#define ROMSIZE1MB              (1 << 20)
#define ROMSIZE512              (1 << 19)
#define ROMSIZE256              (1 << 18)

#define D(x)

static BOOL AMiGAROM_MatchWords(APTR roma, APTR romb)
{
    ULONG *roma_ptr;
    ULONG *romb_ptr;

    /* skip the first 2 ULONG (header) which should match */
    roma_ptr = (ULONG *)((IPTR)roma + (sizeof(IPTR) << 1));
    romb_ptr = (ULONG *)((IPTR)romb + (sizeof(IPTR) << 1));
    
    if ((roma_ptr[0] == romb_ptr[0]) &&
        (roma_ptr[1] == romb_ptr[1]) &&
        (roma_ptr[2] == romb_ptr[2]) &&
        (roma_ptr[3] == romb_ptr[3]))
        return TRUE;
    return FALSE;
}

static BOOL AMiGAROM_IsValid(APTR romimg)
{
    BOOL valid = FALSE;
    ULONG *rom_ptr = (ULONG *)romimg;

    /* check we have a valid rom signature. .. */
    if ((rom_ptr[0] & 0xFFF00000) == 0x11100000)
    {
        valid = TRUE;
    }
    return valid;
}

/* Where the ROM images live: imgcnt is 1, 2 or 4 and takes that many from the front */
static const ULONG rombases[] = { 0xF80000, 0xE00000, 0xB00000, 0xA80000 };

/* Print where one ROM image is, and where it really is if the MMU remaps it */
static void AMiGAROM_ShowRegion(struct KernelBase *KernelBase, ULONG addr, ULONG size)
{
    APTR physaddr = KrnVirtualToPhysical((APTR)addr);

    bug("ROMInfo:     0x%08x - 0x%08x", addr, addr + size - 1);
    if (physaddr != (APTR)addr)
        bug("@  0x%08x - 0x%08x", physaddr, (IPTR)physaddr + size - 1);
    bug("\n");
}

static int AMiGAROMSupport_Init(struct KernelBase *KernelBase)
{
    struct PlatformData *pd = KernelBase->kb_PlatformData;
    ULONG romsize, imgcnt;
    UBYTE id;

    D(bug("[Kernel:Am68k] %s: platformdata @ 0x%p\n", __func__, pd);)

    id = (UBYTE)ReadGayle();
    if (((id > 0) &&
        AMiGAROM_IsValid((APTR)0xA80000) &&
        !AMiGAROM_MatchWords((APTR)0xA80000, (APTR)0xF80000)) ||
        (((UWORD*)&_rom_start >= (UWORD*)0xA80000 && (UWORD*)&_rom_start <(UWORD*)0xB80000) ||
        ((UWORD*)&_ext_start >= (UWORD*)0xA80000 && (UWORD*)&_ext_start < (UWORD*)0xB80000)))
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
        imgcnt = 1;
    }

    /* Size of one ROM image */
    romsize /= imgcnt;

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
            ULONG n;

            /* System ROM, Extended ROM, then the two Workbench ROM halves */
            for (n = 0; n < imgcnt; n++)
            {
                APTR physaddr = getphysaddr((APTR)rombases[n]);

                if (physaddr != (APTR)rombases[n])
                {
                    rommappings[i] = rombases[n];
                    rommappings[i + 1] = (ULONG)physaddr;
                    i += 2;
                }
            }
        }
        if (i > 0)
        {
            pd->romimg = AllocMem(sizeof(ULONG) * (i + 2), MEMF_ANY);
            CopyMem(rommappings, pd->romimg, sizeof(ULONG) * i);
            pd->romimg[i] = NULL;
            pd->romimg[i + 1] = NULL;
            pd->romsize = romsize;
        }
    }

    bug("ROMInfo: ROM region(s)..\n");
    /* Highest image first, as the mapping order above reversed */
    while (imgcnt-- > 0)
        AMiGAROM_ShowRegion(KernelBase, rombases[imgcnt], romsize);
    return TRUE;
}

ADD2INITLIB(AMiGAROMSupport_Init, 120)
