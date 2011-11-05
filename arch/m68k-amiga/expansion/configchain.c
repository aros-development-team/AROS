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

static void findmbram(struct ExpansionBase *ExpansionBase)
{
	LONG ret;
	ULONG step, start, end;

	if (!(SysBase->AttnFlags & AFF_68020))
		return;
	if ((SysBase->AttnFlags & AFF_68020) && !(SysBase->AttnFlags & AFF_ADDR32))
		return;

	/* High MBRAM */
	step =  0x00100000;
	start = 0x08000000;
	end =   0x7f000000;
	ret = AROS_UFC3(LONG, MemoryTest,
		AROS_UFCA(APTR, start, A0),
		AROS_UFCA(APTR, end, A1),
		AROS_UFCA(ULONG, step, D0));
	if (ret < 0)
		return;
	if (ret > 0) {
		AddMemList(ret, MEMF_KICK | MEMF_LOCAL | MEMF_FAST | MEMF_PUBLIC, 40, (APTR)start, "expansion.memory");
		D(bug("MBRAM @%08x, size %08x\n", start, ret));
	}

	/* Low MBRAM, reversed detection needed */
	step =  0x00100000;
	start = 0x08000000;
	end =   0x01000000;
	for (;;) {
		ret = AROS_UFC3(LONG, MemoryTest,
			AROS_UFCA(APTR, start - step, A0),
			AROS_UFCA(APTR, start, A1),
			AROS_UFCA(ULONG, step, D0));
		if (ret <= 0)
			break;
		if (end >= start - step)
			break;
		start -= step;
	}
	if (start != 0x08000000) {
		ULONG size = 0x08000000 - start;
		AddMemList(size, MEMF_KICK | MEMF_LOCAL | MEMF_FAST | MEMF_PUBLIC, 30, (APTR)start, "expansion.memory");
		D(bug("MBRAM @%08x, size %08x\n", start, size));
	}
}

static void allocram(struct ExpansionBase *ExpansionBase)
{
	struct Node *node;
	
	findmbram(ExpansionBase);
	// we should merge address spaces, later..
	D(bug("adding ram boards\n"));
	ForeachNode(&ExpansionBase->BoardList, node) {
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
