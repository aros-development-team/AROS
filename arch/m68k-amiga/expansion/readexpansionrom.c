/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <proto/expansion.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <expansion_intern.h>

static void diagrom(struct ExpansionBase *ExpansionBase, struct ConfigDev *configDev);

/*****************************************************************************

    NAME */
#include <clib/expansion_protos.h>

	AROS_LH2(BOOL, ReadExpansionRom,

/*  SYNOPSIS */
	AROS_LHA(APTR              , board, A0),
	AROS_LHA(struct ConfigDev *, configDev, A1),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 17, Expansion)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    expansion_lib.fd and clib/expansion_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct ExpansionRom *rom = &configDev->cd_Rom;
	ULONG size;
	
	rom->er_Type = ReadExpansionByte(board, 0);
	rom->er_Product = ~ReadExpansionByte(board, 1);
	rom->er_Flags = ~ReadExpansionByte(board, 2);
	rom->er_Reserved03 = ~ReadExpansionByte(board, 3);
	rom->er_Manufacturer = ~((ReadExpansionByte(board, 4) << 8) |
		ReadExpansionByte(board, 4 + 1));
	rom->er_SerialNumber = ~((ReadExpansionByte(board, 6) << 24) |
		(ReadExpansionByte(board, 6 + 1) << 16) |
		(ReadExpansionByte(board, 6 + 2) << 8) |
		(ReadExpansionByte(board, 6 + 3) << 0));
	rom->er_InitDiagVec = ~((ReadExpansionByte(board, 10) << 8) |
		ReadExpansionByte(board, 10 + 1));
	
	if (rom->er_Reserved03 != 0)
		return FALSE;

	if (rom->er_Manufacturer == 0 || rom->er_Manufacturer == 0xffff)
		return FALSE;
	
	if ((rom->er_Type & ERT_TYPEMASK) != ERT_ZORROII && (rom->er_Type & ERT_TYPEMASK) != ERT_ZORROIII)
		return FALSE;
	
	if (rom->er_Flags & ERFF_EXTENDED) {
		size = (16 * 1024 * 1024) << (rom->er_Type & ERT_MEMMASK);
	} else {
		UBYTE mem = rom->er_Type & ERT_MEMMASK;
		if (mem == 0)
			size = 8 * 1024 * 1024; // 8M
		else if (mem <= 4)
			size = (32 * 1024) << mem; // 64k,128k,256k,512k
		else
			size = 32768 << mem; // 1M/2M/4M
	}
	configDev->cd_BoardSize = size;
	configDev->cd_BoardAddr	 = board;
	
	diagrom(ExpansionBase, configDev);

	bug("manufacturer=%d product=%d size=%08x\n", rom->er_Manufacturer, rom->er_Product, size);
	
	return TRUE;

    AROS_LIBFUNC_EXIT
} /* ReadExpansionRom */



static UBYTE getromdata(struct ConfigDev *configDev, UWORD offset)
{
	volatile UBYTE *romb = (UBYTE*)(configDev->cd_BoardAddr + configDev->cd_Rom.er_InitDiagVec);
	volatile UWORD *romw = (UWORD*)(configDev->cd_BoardAddr + configDev->cd_Rom.er_InitDiagVec);
	struct DiagArea *da = configDev->cd_Rom.er_DiagArea;
	
	switch (da->da_Config & DAC_BUSWIDTH)
	{
		case DAC_NIBBLEWIDE:
		{
			offset *= 2;
			return (romb[offset] & 0xf0) | ((romb[offset + 1] & 0xf0) >> 4);
		}
		case DAC_BYTEWIDE:
			return romb[offset];
		case DAC_WORDWIDE:
		{
			UWORD v = romw[offset / 2];
			if (offset & 1)
				return v;
			else
				return v >> 8;
		}
	}
	return 0xff;
}
			
static void diagrom(struct ExpansionBase *ExpansionBase, struct ConfigDev *configDev)
{
	volatile UBYTE *rom = (UBYTE*)(configDev->cd_BoardAddr + configDev->cd_Rom.er_InitDiagVec);
	struct DiagArea *da;

	if (!(configDev->cd_Rom.er_Type & ERTF_DIAGVALID))
		return;
	da = AllocMem(sizeof(struct DiagArea), MEMF_CLEAR | MEMF_PUBLIC);
	if (!da)
		return;
	configDev->cd_Rom.er_DiagArea = da;
	da->da_Config = rom[0];
	da->da_Config = getromdata(configDev, 0);
	da->da_Flags = getromdata(configDev, 1);
	da->da_Size = getromdata(configDev, 2) << 8;
	da->da_Size |= getromdata(configDev, 3) << 0;
	da->da_DiagPoint = getromdata(configDev, 4) << 8;
	da->da_DiagPoint |= getromdata(configDev, 5) << 0;
	da->da_BootPoint = getromdata(configDev, 6) << 8;
	da->da_BootPoint |= getromdata(configDev, 7) << 0;
	da->da_Name = getromdata(configDev, 8) << 8;
	da->da_Name |= getromdata(configDev, 9) << 0;

}

