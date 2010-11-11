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
				if (res->rt_MatchWord == RTC_MATCHWORD && res->rt_MatchTag == res) {
					D(bug("initresident %x\n", res));
					InitResident(res, NULL);
					romptr += sizeof (struct Resident);
				} else {
					romptr += 2;
				}
			}
		}
	}					
}			

static BOOL calldiagrom(struct ExpansionBase *ExpansionBase, struct ExecBase *sb, struct ConfigDev *configDev, UBYTE whenflag)
{
	struct DiagArea *diag = configDev->cd_Rom.er_DiagArea;
	UWORD offset = DAC_CONFIGTIME ? diag->da_DiagPoint : diag->da_BootPoint;
	APTR code = (APTR)(((UBYTE*)diag) + offset);
	BOOL ret;
	
	// call autoconfig ROM da_DiagPoint or bootpoint da_BootPoint
	D(bug("%x %x %x %x\n", code, configDev->cd_BoardAddr, diag, configDev));
	ret = AROS_UFC5(BOOL, code,
   		AROS_UFCA(APTR, configDev->cd_BoardAddr, A0),
		AROS_UFCA(struct DiagArea*, diag, A2),
		AROS_UFCA(struct ConfigDev*, configDev, A3),
		AROS_UFCA(struct ExpansionBase*, ExpansionBase, A5),
		AROS_UFCA(struct ExecBase*, sb, A6));
	return ret;
}

static void callroms(struct ExpansionBase *ExpansionBase, UBYTE whenflag)
{
	struct Node *node;
	D(bug("callroms %x\n", whenflag));
	ForeachNode(&IntExpBase(ExpansionBase)->eb_BoardList, node) {
		struct ConfigDev *configDev = (struct ConfigDev*)node;
		if (configDev->cd_Rom.er_DiagArea && (configDev->cd_Rom.er_DiagArea->da_Config & DAC_BOOTTIME) == whenflag) {
			if (!calldiagrom(ExpansionBase, IntExpBase(ExpansionBase)->eb_SysBase, configDev, whenflag)) {
				FreeMem(configDev->cd_Rom.er_DiagArea, configDev->cd_Rom.er_DiagArea->da_Size);
				configDev->cd_Rom.er_DiagArea = NULL;
			}	
		}
	}
}

// read one byte from expansion autoconfig ROM
static UBYTE getromdata(struct ConfigDev *configDev, UBYTE buswidth, UWORD offset)
{
	volatile UBYTE *rom = (UBYTE*)(configDev->cd_BoardAddr + configDev->cd_Rom.er_InitDiagVec);
	
	switch (buswidth)
	{
		case DAC_NIBBLEWIDE:
			return (rom[offset * 2 + 0] & 0xf0) | ((rom[offset * 2 + 1] & 0xf0) >> 4);
		case DAC_BYTEWIDE:
			return rom[offset * 2];
		case DAC_WORDWIDE:
		default:
			return rom[offset];
	}
}
			
static void diagrom(struct ExpansionBase *ExpansionBase, struct ConfigDev *configDev)
{
	struct DiagArea *da;
	UBYTE buswidth;
	UWORD size, i;

	if (!(configDev->cd_Rom.er_Type & ERTF_DIAGVALID))
		return;
	buswidth = getromdata(configDev, 0, 0) & DAC_BUSWIDTH;
	if (buswidth == DAC_BUSWIDTH) // illegal
		return;
	size = (getromdata(configDev, buswidth, 2) << 8) | (getromdata(configDev, buswidth, 3) << 0);
	
	if (size < sizeof (struct DiagArea))
		return;

	da = AllocMem(size, MEMF_CLEAR | MEMF_PUBLIC);
	if (!da)
		return;

	configDev->cd_Rom.er_DiagArea = da;
	// read rom data, including DiagArea
	for (i = 0; i < size; i++) {
		((UBYTE*)da)[i] = getromdata(configDev, buswidth, i);
	}
}

static void readroms(struct ExpansionBase *ExpansionBase)
{
	struct Node *node;
	ForeachNode(&IntExpBase(ExpansionBase)->eb_BoardList, node) {
		struct ConfigDev *configDev = (struct ConfigDev*)node;
		diagrom(ExpansionBase, configDev);
	}
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
			}
			AddMemList(size, attr, pri, addr, "Fast Memory");
			configDev->cd_Flags |= CDF_PROCESSED;
		}
	}
}
	

AROS_LH1(void, ConfigChain,
	AROS_LHA(APTR, baseAddr, A0),
	struct ExpansionBase *, ExpansionBase, 11, Expansion)
{
    AROS_LIBFUNC_INIT

	struct ConfigDev *configDev = NULL;

	if (baseAddr == 0) {
		// called by strap
		// this is not right, real expansion most likely does this earlier
		callroms(ExpansionBase, DAC_CONFIGTIME);
		romtaginit(ExpansionBase);
		callroms(ExpansionBase, DAC_BINDTIME);
		return;
	}

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

	readroms(ExpansionBase);

    AROS_LIBFUNC_EXIT
} /* ConfigChain */
