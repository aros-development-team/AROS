/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/expansion.h>

#include "expansion_intern.h"

#include <clib/expansion_protos.h>
#include <proto/exec.h>
#include <exec/resident.h>
#include <aros/asmcall.h>

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
	

// ROMTAG INIT time
static void romtaginit(struct ExpansionBase *ExpansionBase)
{
	struct Node *node;
	// look for possible romtags in expansion ROM image and InitResident() them if found
	D(bug("romtaginit\n"));
	ObtainConfigBinding();
	ForeachNode(&IntExpBase(ExpansionBase)->eb_BoardList, node) {
		struct ConfigDev *configDev = (struct ConfigDev*)node;
		if (configDev->cd_Rom.er_DiagArea && (configDev->cd_Rom.er_DiagArea->da_Config & DAC_BOOTTIME) == DAC_CONFIGTIME && (configDev->cd_Flags & CDF_CONFIGME)) {
			struct Resident *res;
			UWORD *romptr = (UWORD*)configDev->cd_Rom.er_DiagArea;
			UWORD *romend = (UWORD*)(((UBYTE*)configDev->cd_Rom.er_DiagArea) + configDev->cd_Rom.er_DiagArea->da_Size - 26); // 26 = real sizeof(struct Resident)!
			struct CurrentBinding cb = {
			    .cb_ConfigDev = configDev
			};
			SetCurrentBinding(&cb, sizeof(cb));
			while (romptr <= romend) {
				res = (struct Resident*)romptr;
				if (res->rt_MatchWord == RTC_MATCHWORD && res->rt_MatchTag == res) {
					D(bug("Diag board %08x InitResident %08x (%d %02x '%s')\n",
						configDev->cd_BoardAddr, res, res->rt_Pri, res->rt_Flags, res->rt_Name));
					InitResident(res, BNULL);
					break; /* must not keep looking */
				}
				romptr++;
			}
		}
	}
	ReleaseConfigBinding();
	D(bug("romtaginit done\n"));
}

AROS_UFP3(ULONG, MemoryTest,
    AROS_UFPA(APTR, startaddr, A0),
    AROS_UFPA(APTR, endaddr, A1),
    AROS_UFPA(ULONG, block, D0));

static ULONG autosize(struct ExpansionBase *ExpansionBase, struct ConfigDev *configDev)
{
	UBYTE sizebits = configDev->cd_Rom.er_Flags & ERT_Z3_SSMASK;
	ULONG maxsize = configDev->cd_BoardSize;
	ULONG size = 0;
	UBYTE *addr = (UBYTE*)configDev->cd_BoardAddr;

	D(bug("sizebits=%x\n", sizebits));
	if (sizebits >= 14) /* 14 and 15 = reserved */
		return 0;
	if (sizebits >= 2 && sizebits <= 8)
		return 0x00010000 << (sizebits - 2);
	if (sizebits >= 9)
		return 0x00600000 + (0x200000 * (sizebits - 9));
	size = AROS_UFC3(ULONG, MemoryTest,
		AROS_UFCA(APTR, addr, A0),
		AROS_UFCA(APTR, addr + maxsize, A1),
		AROS_UFCA(ULONG, 0x80000, D0));
	D(bug("addr=%lx size=%lx maxsize=%lx\n", addr, size, maxsize));
	return size;
}

static void allocram(struct ExpansionBase *ExpansionBase)
{
	struct Node *node;
	
	// we should merge address spaces, later..
	D(bug("adding ram boards\n"));
	ForeachNode(&IntExpBase(ExpansionBase)->eb_BoardList, node) {
		struct ConfigDev *configDev = (struct ConfigDev*)node;
		if ((configDev->cd_Rom.er_Type & ERTF_MEMLIST) && !(configDev->cd_Flags & CDF_SHUTUP) && !(configDev->cd_Flags & CDF_PROCESSED)) {
			ULONG attr = MEMF_PUBLIC | MEMF_FAST | MEMF_KICK;
			ULONG size = configDev->cd_BoardSize;
			APTR addr = configDev->cd_BoardAddr;
			LONG pri = 20;
			if (configDev->cd_BoardAddr <= (APTR)0x00FFFFFF) {
				attr |= MEMF_24BITDMA;
				pri = 0;
			} else if ((configDev->cd_Rom.er_Flags & ERT_Z3_SSMASK) != 0) {
				size = autosize(ExpansionBase, configDev);
			}
			if (size && size <= configDev->cd_BoardSize) {
				D(bug("ram board at %08x, size %08x attr %08x\n", addr, size, attr));
				AddMemList(size, attr, pri, addr, "Fast Memory");
			}
			configDev->cd_Flags |= CDF_PROCESSED;
		}
	}
	D(bug("ram boards done\n"));
}
	

AROS_LH1(void, ConfigChain,
	AROS_LHA(APTR, baseAddr, A0),
	struct ExpansionBase *, ExpansionBase, 11, Expansion)
{
    AROS_LIBFUNC_INIT

	struct ConfigDev *configDev = NULL;

	if (baseAddr == 0) {
		// called by strap
		romtaginit(ExpansionBase);

		// enable 68040+ data caches and 68060 superscalar mode
		// this is not the right place but we can't enable them
		// any earlier (memory detection, boot roms that breaks if
		// full 68060 caching enabled)
		if (SysBase->AttnFlags & AFF_68060) {
			Supervisor((ULONG_FUNC)Enable68060SuperScalar);
		} else if (SysBase->AttnFlags & AFF_68040) {
			CacheControl(CACRF_EnableD, CACRF_EnableD);
		} else if (SysBase->AttnFlags & AFF_68030) {
			ULONG tc = Supervisor((ULONG_FUNC)Check68030MMU);
			if (tc & (1 << 31)) { /* Only if MMU enabled */
				CacheControl(CACRF_EnableD, CACRF_EnableD);
			}
		}
		return;
	}
	/* Try to guess if we have Z3 based machine.
	 * Not all Z3 boards appear in Z2 region.
	 *
	 * Ignores original baseAddr by design.
	 */
	BOOL maybeZ3 = (SysBase->AttnFlags & AFF_ADDR32);
	D(bug("configchain\n"));
	for(;;) {
		BOOL gotrom = FALSE;
		if (!configDev)
			configDev = AllocConfigDev();
		if (!configDev)
			break;
		if (maybeZ3) {
			baseAddr = (APTR)EZ3_EXPANSIONBASE;
			gotrom = ReadExpansionRom(baseAddr, configDev);
		}
		if (!gotrom) {
			baseAddr = (APTR)E_EXPANSIONBASE;
			gotrom = ReadExpansionRom(baseAddr, configDev);
		}
		if (!gotrom) {
			FreeConfigDev(configDev);
			break;
		}
		if (ConfigBoard(baseAddr, configDev)) {
			AddConfigDev(configDev);
			configDev = NULL;
		}
	}
	D(bug("configchain done\n"));

	allocram(ExpansionBase);

    AROS_LIBFUNC_EXIT
} /* ConfigChain */
