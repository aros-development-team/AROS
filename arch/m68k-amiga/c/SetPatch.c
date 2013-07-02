
/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */
 
#include <exec/types.h>
#include <exec/execbase.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/setpatch.h>
#include <proto/kernel.h>
#include <proto/graphics.h>

#define SH_GLOBAL_DOSBASE 1
#define SH_GLOBAL_SYSBASE 1

#include <aros/shcommands.h>

const TEXT version[] = "$VER: SetPatch AROS-m68k 41.3 (" ADATE ")\n";

void Enable68060SuperScalar(void);
asm (
    ".text\n"
    ".global Enable68060SuperScalar\n"
    ".func Enable68060SuperScalar\n"
    "Enable68060SuperScalar:\n"
    /* enable supercalar */
    "dc.l 0x4e7a0808\n" // movec %pcr,%d0
    "bset #0,%d0\n"
    "dc.l 0x4e7b0808\n" // movec %d0,%pcr
    /* enable code&data caches, store buffer and branch cache */
    "dc.l 0x4e7a0002\n" // movec %cacr,%d0
    "or.l #0xa0808000,%d0\n"
    "dc.l 0x4e7b0002\n" // movec %d0,%cacr
    "rte\n"
    ".endfunc\n"
);

ULONG Check68030MMU(void);
asm (
    ".text\n"
    ".chip 68030\n"
    ".global Check68030MMU\n"
    ".func Check68030MMU\n"
    "Check68030MMU:\n"
    "subq.l #4,%sp\n"
    "pmove %tc,(%sp)\n"
    "move.l (%sp),%d0\n"
    "addq.l #4,%sp\n"
    "rte\n"
    ".endfunc\n"
);

APTR getvbr(void);
asm (
    ".text\n"
    ".chip 68010\n"
    ".global getvbr\n"
    ".func getvbr\n"
    "getvbr:\n"
    "movec %vbr,%d0\n"
    "rte\n"
    ".endfunc\n"
);

static void setvbr(APTR vbr)
{
    asm volatile (
    ".chip 68010\n"
    "move.l %0,%%d0\n"
    "movem.l %%a5/%%a6,-(%%sp)\n"
    "move.l 4.w,%%a6\n"
    "lea 1f(%%pc),%%a5\n"
    "jsr -0x1e(%%a6)\n"
    "movem.l (%%sp)+,%%a5/%%a6\n"
    "bra.s 0f\n"
    "1:\n"
    "movec %%d0,%%vbr\n"
    "rte\n"
    "0:\n"
    : : "m" (vbr)
    );
}

struct mmuportnode
{
    struct Node node;
    ULONG addr;
    ULONG len;
    ULONG flags;
};

#define PAGE_SIZE 4096

static void fastvbr(BOOL quiet)
{
    APTR oldvbr, newvbr;

    if (!(SysBase->AttnFlags & AFF_68010))
        return;
    Disable();
    oldvbr = (APTR)Supervisor((ULONG_FUNC)getvbr);
    Enable();
    if (!oldvbr) {
        newvbr = AllocMem(4 * 256, MEMF_FAST | MEMF_CLEAR);
        if (!newvbr)
            return;
        CopyMemQuick(oldvbr, newvbr, 4 * 256);
        Disable();
        setvbr(newvbr);
        Enable();
        oldvbr = newvbr;
    }
    if (!quiet)
        Printf("VBR moved to Fast RAM at %p\n", oldvbr);
}

static void p5stuff(BOOL quiet)
{
    APTR KernelBase;
    UBYTE *mmuport;
    struct mmuportnode *node;
    struct List *list;

    mmuport = (UBYTE*)FindPort("BOOT-MMU-Port");
    if (!mmuport)
        return;
    if (!quiet)
        Printf("Phase 5 BOOT-MMU-Port found at %p\n", mmuport);
    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
        return;
    list = (struct List*)(mmuport + 0x26);
    ForeachNode(list, node) {
        ULONG addr = node->addr, end;
        ULONG len = node->len; 
        if (!quiet)
            Printf("- %p %08lx %08lx\n", addr, len, node->flags);
        end = addr + len;
        addr &= ~(PAGE_SIZE - 1);
        end += (PAGE_SIZE - 1);
        end &= ~(PAGE_SIZE - 1);
        Disable();
        KrnSetProtection((void*)addr, end - addr, MAP_Readable | MAP_Writable | MAP_Executable | MAP_CacheInhibit);
        Enable();
    }
}
 

static void mmusetup(BOOL quiet)
{
    // enable 68040+ data caches and 68060 superscalar mode
    // copyback is also enabled if MMU setup was done by rom code
    CacheClearU();
    Disable();
    if (SysBase->AttnFlags & AFF_68060) {
        Supervisor((ULONG_FUNC)Enable68060SuperScalar);
    } else if (SysBase->AttnFlags & AFF_68040) {
        CacheControl(CACRF_EnableD, CACRF_EnableD);
    } else if (SysBase->AttnFlags & AFF_68030) {
        ULONG tc = Supervisor((ULONG_FUNC)Check68030MMU);
        if (tc & (1 << 31)) { /* Only if MMU enabled */
            CacheControl(CACRF_EnableD | CACRF_WriteAllocate, CACRF_EnableD | CACRF_WriteAllocate);
        }
    }
    Enable();
}

extern void patches(BOOL, ULONG);

AROS_SH6H(SetPatch, 41.4, "AROS SetPatch (m68k)",
    AROS_SHAH(BOOL, Q=,       QUIET, /S, FALSE, "Be quiet"),
    AROS_SHAH(BOOL, NOCA=,  NOCACHE, /S, FALSE, "Don't install cache patches"),
    AROS_SHAH(BOOL, NOCO=,NOCOPYMEM, /S, FALSE, "Don't install CopyMem patches"),
    AROS_SHAH(BOOL, NOV=, NOVBRMOVE, /S, FALSE, "Don't move the VBR to MEMF_FAST"),
    AROS_SHAH(BOOL, NOAGA=,   NOAGA, /S, FALSE, "Don't enable AGA modes"),
    AROS_SHAH(BOOL, DMMU=, DEBUGMMU, /S, FALSE, "MMU protect first page"))
{
    AROS_SHCOMMAND_INIT

    struct Library *SetPatchBase;
    struct GfxBase *GfxBase;

    /* NOTE: This is currently a 'am I running on AROS' test, but
     *       we should use SetPatch/AddPatch() one day
     */
    if ((SetPatchBase = OpenLibrary("setpatch.library", 41))) {
        BOOL justinstalled680x0 = FALSE;
        BOOL installed680x0 = FALSE;
        BOOL x68040 = FALSE, x68060 = FALSE;

        GfxBase = (struct GfxBase*)OpenLibrary("graphics.library", 0);
        if (SysBase->AttnFlags & (AFF_68040 | AFF_68060)) {
            BOOL ox68040 = FALSE, ox68060 = FALSE;

            Forbid();
            if (FindName(&SysBase->LibList, "68040.library"))
                ox68040 = TRUE;
            if (FindName(&SysBase->LibList, "68060.library"))
                ox68060 = TRUE;
            Permit();
            CloseLibrary(OpenLibrary("680x0.library", 0));
            Forbid();
            if (FindName(&SysBase->LibList, "68040.library"))
                x68040 = TRUE;
            if (FindName(&SysBase->LibList, "68060.library"))
                x68060 = TRUE;
            Permit();

            if ((!ox68040 && !ox68060) && (x68040 || x68060))
                justinstalled680x0 = TRUE;
            if (x68040 || x68060)
                installed680x0 = TRUE;
        }

        if (!SHArg(NOVBRMOVE))
            fastvbr(SHArg(QUIET));

        if (justinstalled680x0)
            patches(SHArg(QUIET), SHArg(NOCOPYMEM) ? 0 : 1);
 
        if (!SHArg(NOCACHE)) {
            if (justinstalled680x0)
                p5stuff(SHArg(QUIET));
            mmusetup(SHArg(QUIET));
        } else {
            CacheControl(0, CACRF_EnableD | CACRF_CopyBack | CACRF_DBE);
        }

        if (!SHArg(NOAGA))
            SetChipRev(SETCHIPREV_BEST);

        if (SHArg(DEBUGMMU)) {
            /* Mark first page invalid, special handler only accepts ExecBase reads */
            void *KernelBase = OpenResource("kernel.resource");
            if (KernelBase) {
                KrnSetProtection(0, PAGE_SIZE, 0);
            }
        }

        if (SHArg(QUIET) == FALSE) {
            ULONG flags;
            if (installed680x0) {
                Printf("%ld Support Code Loaded\n", x68060 ? 68060 : 68040);
            } else if (SysBase->AttnFlags & (AFF_68040 | AFF_68060)) {
                Printf("WARNING: %ld CPU without LIBS:680x0.library.\n", (SysBase->AttnFlags & AFF_68060) ? 68060 : 68040);
            }
            flags = CacheControl(0, 0);
            if (flags & CACRF_EnableD)
                Printf("Data Cache Enabled\n");
            if (flags & CACRF_CopyBack)
                Printf("CopyBack Enabled\n");
            if ((GfxBase->ChipRevBits0 & SETCHIPREV_AA) ==  SETCHIPREV_AA)
                Printf("Enabled Advanced Graphics Modes\n");
        }
        CloseLibrary(GfxBase);
        CloseLibrary(SetPatchBase);
    }
    return (SetPatchBase != NULL) ? RETURN_OK : RETURN_FAIL;

    AROS_SHCOMMAND_EXIT
}
