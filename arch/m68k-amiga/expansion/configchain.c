/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <proto/expansion.h>

#include "expansion_intern.h"

#include <clib/expansion_protos.h>

#include <proto/exec.h>

static void allocram(struct ExpansionBase *ExpansionBase)
{
	struct Node *node;
	
	// we should merge address spaces, later..
	ForeachNode(&IntExpBase(ExpansionBase)->eb_BoardList, node) {
		struct ConfigDev *configDev = (struct ConfigDev*)node;
		if ((configDev->cd_Rom.er_Type & ERTF_MEMLIST) && !(configDev->cd_Flags & CDF_SHUTUP) && !(configDev->cd_Flags & CDF_PROCESSED)) {
			ULONG attr = MEMF_PUBLIC | MEMF_FAST;
			ULONG size = configDev->cd_BoardSize;
			APTR addr = configDev->cd_BoardAddr;
			LONG pri = 10;
			if (configDev->cd_BoardAddr <= (APTR)0x00FFFFFF) {
				attr |= MEMF_24BITDMA;
				pri = 5;
			}
			AddMemList(size, attr, pri, addr, "fast memory");
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
