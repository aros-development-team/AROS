/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>
#include <proto/expansion.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <expansion_intern.h>

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
	
	if ((rom->er_Type & ERT_TYPEMASK) == ERT_ZORROIII && (rom->er_Flags & ERFF_EXTENDED)) {
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
	
	D(bug("Found board: mfg=%d prod=%d size=%08x serial=%08x diag=%p\n",
		rom->er_Manufacturer, rom->er_Product, size, rom->er_SerialNumber, rom->er_InitDiagVec));
	
	return TRUE;

    AROS_LIBFUNC_EXIT
} /* ReadExpansionRom */



