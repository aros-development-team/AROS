
/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */
 
 /* Outputs current MMU table setup. AOS compatible. */

#include <exec/types.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <stdio.h>

#define PROTO_KERNEL_H      /* Don't pick up AROS kernel hooks */

#define MMU030 1
#define MMU040 2
#define MMU060 3

#define LEVELA_SIZE 7
#define LEVELB_SIZE 7
#define LEVELC_SIZE 6
#define PAGE_SIZE 12 // = 1 << 12 = 4096

/* Macros that hopefully make MMU magic a bit easier to understand.. */

#define LEVELA_VAL(x) ((((ULONG)(x)) >> (32 - (LEVELA_SIZE                            ))) & ((1 << LEVELA_SIZE) - 1))
#define LEVELB_VAL(x) ((((ULONG)(x)) >> (32 - (LEVELA_SIZE + LEVELB_SIZE              ))) & ((1 << LEVELB_SIZE) - 1))
#define LEVELC_VAL(x) ((((ULONG)(x)) >> (32 - (LEVELA_SIZE + LEVELB_SIZE + LEVELC_SIZE))) & ((1 << LEVELC_SIZE) - 1))

#define LEVELA(root, x) (root[LEVELA_VAL(x)])
#define LEVELB(a, x) (((ULONG*)(((ULONG)a) & ~((1 << (LEVELB_SIZE + 2)) - 1)))[LEVELB_VAL(x)])
#define LEVELC(b, x) (((ULONG*)(((ULONG)b) & ~((1 << (LEVELC_SIZE + 2)) - 1)))[LEVELC_VAL(x)])

#define ISINVALID(x) ((((ULONG)x) & 3) == 0)

struct mmu68040
{
    ULONG *srp;
    ULONG *urp;
    ULONG itt0, itt1;
    ULONG dtt0, dtt1;
    ULONG tc;
    ULONG pcr;
};
struct mmu68030
{
    ULONG crp[2];
    ULONG tt0;
    ULONG tt1;
    ULONG tc;
};

static UBYTE mmutype;
struct mmu68040 m68040s;
struct mmu68030 m68030s;
struct DosLibrary *DOSBase;

static ULONG getdesc(ULONG *root, ULONG addr)
{
	ULONG desc;

	desc = LEVELA(root, addr);
	if (ISINVALID(desc))
		return desc;
	desc = LEVELB(desc, addr);
	if (ISINVALID(desc))
		return desc;
	desc = LEVELC(desc, addr);
	return desc;
}

static void get68040(void)
{
    asm volatile (
    	".chip 68040\n"
    	"lea m68040s,%%a0\n"
    	"movec %%srp,%%a1\n"
    	"move.l %%a1,(%%a0)+\n"
    	"movec %%urp,%%a1\n"
    	"move.l %%a1,(%%a0)+\n"
    	"movec %%itt0,%%a1\n"
    	"move.l %%a1,(%%a0)+\n"
    	"movec %%itt1,%%a1\n"
    	"move.l %%a1,(%%a0)+\n"
    	"movec %%dtt0,%%a1\n"
    	"move.l %%a1,(%%a0)+\n"
    	"movec %%dtt1,%%a1\n"
    	"move.l %%a1,(%%a0)+\n"
    	"movec %%tc,%%a1\n"
    	"move.l %%a1,(%%a0)+\n"
  	"rte\n"
    	: : : "a0", "a1");
};

static void get68060(void)
{
    asm volatile (
    	".chip 68060\n"
    	"lea m68040s,%%a0\n"
    	"movec %%pcr,%%a1\n"
    	"move.l %%a1,7*4(%%a0)\n"
    	"rte\n"
    	: : : "a0", "a1");
};

static void get68030(void)
{
#if 0
    asm volatile (
    	".chip 68030\n"
     	"move.l #m68030,%%a0\n"
    	"pmove %%crp,(%%a0)\n"
    	"pmove %%tt0,8(%%a0)\n"
    	"pmove %%tt1,12(%%a0)\n"
  	"pmove %%tc,16(%a0)\n"
  	"rte\n"
    	: : : "a0");
#endif
};

static void dump_descriptor(ULONG desc)
{
    UBYTE cm = (desc & (0x040 | 0x020)) >> 5;
    Printf ((desc & 0x400) ? " G" : " -");
    Printf ((desc & 0x080) ? "S" : "-");
    Printf ((desc & 0x004) ? "W " : "- ");
    if (cm == 0)
	Printf("WT");
    else if (cm == 1)
	Printf("CB");
    else if (cm == 2)
	Printf("IP");
    else
	Printf("II");
}

// Ugnore M and U
#define IGNOREMASK (0x10 | 0x08)

static void dump_mmu(ULONG *root)
{
	ULONG i;
	ULONG startaddr;
	ULONG odesc;
	ULONG totalpages;
	ULONG pagemask = (1 << PAGE_SIZE) - 1;
	
	totalpages = 1 << (32 - PAGE_SIZE);
	startaddr = 0;
	odesc = getdesc(root, startaddr);
	for (i = 0; i <= totalpages; i++) {
		ULONG addr = i << PAGE_SIZE;
		ULONG desc = 0;
		if (i < totalpages)
			desc = getdesc(root, addr);
		if ((desc & (pagemask & ~IGNOREMASK)) != (odesc & (pagemask & ~IGNOREMASK)) || i == totalpages) {
			Printf("%08lx - %08lx: %08lx", startaddr, addr - 1, odesc);
			if (!ISINVALID(odesc)) {
			    if (mmutype >= MMU040) {
			        if ((odesc & 3) == 2) {
			            ULONG idesc = *((ULONG*)(odesc & ~3));
			            Printf(" -> %08lx", idesc);
			            dump_descriptor (idesc);
			            Printf(" %08lx", idesc & ~pagemask);
			        } else {
			            dump_descriptor (odesc);
			            Printf(" %08lx", odesc & ~pagemask);
			        }
			    } else {
			        Printf(" %08lx", odesc & ~pagemask);
			    }
			} else {
			    Printf(" INV");
			}
			Printf("\n");
			startaddr = addr;
			odesc = desc;
		}
	}
}			

__startup static AROS_PROCH(startup, argstr, argsize, SysBase)
{
    AROS_PROCFUNC_INIT

    DOSBase = (APTR)OpenLibrary("dos.library", 0);

    if (Output() == BNULL)
        goto end;

    if (!(SysBase->AttnFlags & AFF_68030)) {
        Printf("68030 or better required\n");
        goto end;
    }
    mmutype = (SysBase->AttnFlags & AFF_68040) ? MMU040 : MMU030;
    Supervisor((ULONG_FUNC)(mmutype == MMU030 ? get68030 : get68040));
    
    if (mmutype >= MMU040) {
        Printf("SRP:  %08lx URP:  %08lx\n", m68040s.srp, m68040s.urp);
        Printf("ITT0: %08lx ITT1: %08lx\n", m68040s.itt0, m68040s.itt1);
        Printf("DTT0: %08lx DTT1: %08lx\n", m68040s.dtt0, m68040s.dtt1);
        Printf("TC  : %08lx\n", m68040s.tc);
        if (SysBase->AttnFlags & AFF_68060) {
            Supervisor((ULONG_FUNC)get68060);
            Printf("PCR : %08lx\n", m68040s.pcr);
        }   
    
        if ((m68040s.tc & 0xc000) == 0x8000) {
            if (m68040s.srp != m68040s.urp)
                Printf("SRP dump:\n");
            else
                Printf("MMU dump:\n");
            dump_mmu(m68040s.srp);
            if (m68040s.srp != m68040s.urp) {
                Printf("URP dump:\n");
                dump_mmu(m68040s.urp);
            }
        }
    } else {
        Printf("CRP: %08lx %08lx\n", m68030s.crp[0], m68030s.crp[1]);
        Printf("TT0: %08lx TT1: %08lx\n", m68030s.tt0, m68030s.tt1);
        Printf("TC : %08lx\n", m68030s.tc);
    }

end:
    CloseLibrary((APTR)DOSBase);
    return 0;

    AROS_PROCFUNC_EXIT
}

   
