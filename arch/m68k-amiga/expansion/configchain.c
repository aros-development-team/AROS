/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/expansion.h>

#include "expansion_intern.h"

#include <clib/expansion_protos.h>

#include <proto/exec.h>
#include <exec/resident.h>

// ROMTAG INIT time
static void romtaginit(struct ExpansionBase *ExpansionBase)
{
	struct Node *node;
	// look for possible romtags in expansion ROM image and InitResident() them if found
	D(bug("romtaginit\n"));
	ForeachNode(&IntExpBase(ExpansionBase)->eb_BoardList, node) {
		struct ConfigDev *configDev = (struct ConfigDev*)node;
		if (configDev->cd_Rom.er_DiagArea && (configDev->cd_Rom.er_DiagArea->da_Config & DAC_BOOTTIME) == DAC_CONFIGTIME) {
			struct Resident *res;
			UWORD *romptr = (UWORD*)configDev->cd_Rom.er_DiagArea;
			UWORD *romend = (UWORD*)(((UBYTE*)configDev->cd_Rom.er_DiagArea) + configDev->cd_Rom.er_DiagArea->da_Size);
			SetCurrentBinding(&configDev, 4);
			while (romptr < romend - sizeof (struct Resident*)) {
				res = (struct Resident*)romptr;
				if (res->rt_MatchWord == RTC_MATCHWORD && res->rt_MatchTag == res && res->rt_Pri < 105) {
					D(bug("initresident %x '%s'\n", res, res->rt_Name));
					InitResident(res, NULL);
					romptr += 13; //sizeof (struct Resident);
				} else {
					romptr += 1;
				}
			}
		}
	}
}

extern UBYTE _rom_start;
extern UBYTE _ext_start;

static ULONG checkramrom(UBYTE *addr, ULONG size, ULONG mask)
{
	/* check if our "rom" is loaded to ram */
	if (addr < &_rom_start && addr + size > &_rom_start)
		size = (&_rom_start - addr) & ~mask;
	return size;
}

static ULONG autosize(struct ExpansionBase *ExpansionBase, struct ConfigDev *configDev)
{
	UBYTE sizebits = configDev->cd_Rom.er_Flags & ERT_Z3_SSMASK;
	ULONG maxsize = configDev->cd_BoardSize;
	ULONG size = 0;
	volatile ULONG *addr = (ULONG*)configDev->cd_BoardAddr;
	volatile ULONG *startaddr = addr;
	ULONG testpattern1 = 0xa5fe, testpattern2 = 0xf15a;
	ULONG starttmp;
	ULONG stepsize = 0x80000;

	D(bug("sizebits=%x\n", sizebits));
	if (sizebits >= 14) /* 14 and 15 = reserved */
		return 0;
	if (sizebits >= 2 && sizebits <= 8)
		return 0x00010000 << (sizebits - 2);
	if (sizebits >= 9)
		return 0x00600000 + (0x200000 * (sizebits - 9));
	maxsize = checkramrom((UBYTE*)addr, maxsize, 0x7ffff);
	if (!maxsize)
		return 0;
	starttmp = startaddr[0];
	startaddr[0] = 0;
	for (;;) {
		ULONG tmp1, tmp2, tmp3;
		tmp1 = addr[0];
		/* make sure possible data cache entry gets reset */
		addr[0] = testpattern2;
		tmp2 = addr[0];
		addr[0] = testpattern1;
		tmp2 = startaddr[0];
		tmp3 = addr[0];
		addr[0] = tmp1;
		if (tmp3 != testpattern1) {
			D(bug("test pattern mismatch at %p\n", addr));
			break;
		}
		if (size && tmp2 == testpattern1) {
			/* wrap around? */
			D(bug("wrap around at %p\n", addr));
			break;
		}
		size += stepsize;
		addr += stepsize / sizeof(ULONG);
		if (size >= maxsize)
			break;
	}
	startaddr[0] = starttmp;
	D(bug("size=%x maxsize=%x\n", size, maxsize));
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
			ULONG attr = MEMF_PUBLIC | MEMF_FAST;
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
				size = checkramrom(addr, size, 65535);
				if (size) {
				    D(bug("ram board at %08x, size %08x attr %08x\n", addr, size, attr));
				    AddMemList(size, attr, pri, addr, "Fast Memory");
				}
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
		return;
	}
bug("configchain\n");
	for(;;) {
		if (!configDev)
			configDev = AllocConfigDev();
		if (!configDev)
			break;
		if (!ReadExpansionRom(baseAddr, configDev)) {
			FreeConfigDev(configDev);
			break;
		}
		if (ConfigBoard(baseAddr, configDev)) {
			AddTail(&IntExpBase(ExpansionBase)->eb_BoardList, &configDev->cd_Node);
			configDev = NULL;
		}
	}

	allocram(ExpansionBase);

    AROS_LIBFUNC_EXIT
} /* ConfigChain */
