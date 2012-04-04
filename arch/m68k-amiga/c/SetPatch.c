
/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */
 
#include <exec/types.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>

#define TEMPLATE "QUIET/S,NOCACHE/S"
#define ARG_QUIET   0
#define ARG_NOCACHE 1
#define ARG_NUM 2

/* "SetPatch AROS-m68k" is magic string that is checked in LoadSeg() compatibility hack.
 * We can't allow original AOS C:SetPatch to run because it pokes undocumented structures.
 * Check arch/m68k-all/dos/bcpl_patches.c
 */
const TEXT version[] = "$VER: SetPatch AROS-m68k 41.1 (" ADATE ")\n";

int __nocommandline;

static void Enable68060SuperScalar(void)
{
    asm volatile (
	".text\n"
	/* enable supercalar */
	"dc.l	0x4e7a0808\n"	// movec %pcr,%d0
	"bset	#0,%d0\n"
	"dc.l	0x4e7b0808\n"	// movec %d0,%pcr
	/* enable code&data caches, store buffer and branch cache */
	"dc.l	0x4e7a0002\n"	// movec %cacr,%d0
	"or.l	#0xa0808000,%d0\n"
	"dc.l	0x4e7b0002\n"	// movec %d0,%cacr
	"rte\n"
    );
}
static ULONG Check68030MMU(void)
{
    register UWORD ret asm("%d0");
    asm volatile (
    	".chip 68030\n"
    	"subq.l	#4,%%sp\n"
    	"pmove	%%tc,(%%sp)\n"
    	"move.l	(%%sp),%%d0\n"
    	"addq.l	#4,%%sp\n"
    	"rte\n"
    	: "=r" (ret)
    );
    return ret;
};

struct mmuportnode
{
    struct Node node;
    ULONG addr;
    ULONG len;
    ULONG flags;
};

#define PAGE_SIZE 4096

static void p5stuff (BOOL quiet)
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
    p5stuff(quiet);
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

int main (void)
{
    struct RDArgs *rda;
    IPTR args[ARG_NUM] = { FALSE, FALSE };

    rda = ReadArgs(TEMPLATE, args, NULL);
    if (rda) {
        BOOL x68040 = FALSE, x68060 = FALSE;

        CloseLibrary(OpenLibrary("680x0.library", 0));
        Forbid();
        if (FindName(&SysBase->LibList, "68040.library"))
            x68040 = TRUE;
        if (FindName(&SysBase->LibList, "68060.library"))
            x68060 = TRUE;
        Permit();
 
        if (args[ARG_NOCACHE] == FALSE) {
            if (!(SysBase->AttnFlags & (AFF_68040 | AFF_68060)) || x68040 || x68060)
                mmusetup(args[ARG_QUIET]);
        } else {
            CacheControl(0, CACRF_EnableD | CACRF_CopyBack | CACRF_DBE);
        }
        if (args[ARG_QUIET] == FALSE) {
            ULONG flags;
            if (x68040 || x68060) {
                Printf("%ld Support Code Loaded\n", x68060 ? 68060 : 68040);
            } else if (SysBase->AttnFlags & (AFF_68040 | AFF_68060)) {
                Printf("WARNING: %ld CPU without LIBS:680x0.library.\n", (SysBase->AttnFlags & AFF_68060) ? 68060 : 68040);
            }
            flags = CacheControl(0, 0);
            if (flags & CACRF_EnableD)
                Printf("Data Caches Enabled\n");
            if (flags & CACRF_CopyBack)
                Printf("CopyBack Enabled\n");
        }
        
        FreeArgs(rda);
    }
    return 0;
}

   