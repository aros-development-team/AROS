/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "expansion_intern.h"

#include <proto/expansion.h>

/*****************************************************************************

    NAME */
#include <clib/expansion_protos.h>

	AROS_LH2(BOOL, ConfigBoard,

/*  SYNOPSIS */
	AROS_LHA(APTR              , board, A0),
	AROS_LHA(struct ConfigDev *, configDev, A1),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 10, Expansion)

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

	UBYTE type = configDev->cd_Rom.er_Type & ERT_TYPEMASK;
	BOOL memorydevice = configDev->cd_Rom.er_Type & ERTF_MEMLIST;
	UBYTE *space;
	ULONG align;
	ULONG size = configDev->cd_BoardSize;
	IPTR start, end, addr;
	
	if (type == ERT_ZORROII) {
		start = 0x00200000;
		end   = 0x009FFFFF;
		space = IntExpBase(ExpansionBase)->eb_z2Slots;
		align = configDev->cd_BoardSize;
	} else {
		start = 0x10000000;
		end   = 0x7FFFFFFF;
		space = IntExpBase(ExpansionBase)->eb_z3Slots;
		align = 0x00100000;
	}
	if (!memorydevice) {
		start = 0x00E90000;
		end   = 0x00EFFFFF;
		align = 0x00010000;
	}

	for (addr = start; addr < end; addr += align) {
		IPTR startaddr = addr;
		UWORD offset = startaddr / (E_SLOTSIZE * SLOTSPERBYTE);
		UBYTE bit = (startaddr / E_SLOTSIZE) % SLOTSPERBYTE;
		UBYTE res = space[offset];
		ULONG sizeleft = size;

		if (res & (7 - (1 << bit)))
			continue;

		// found free start address
		if (size >= E_SLOTSIZE * SLOTSPERBYTE) {
			// needs at least 1 byte and is always aligned to byte
			while (space[offset] == 0 && sizeleft > 0 && offset <= end / (E_SLOTSIZE * SLOTSPERBYTE)) {
				offset++;
				sizeleft -= E_SLOTSIZE * SLOTSPERBYTE;
			}
		} else {
			// bit by bit small board check (fits in one byte)
			while ((res & (7 - (1 << bit))) == 0 && sizeleft > 0) {
				sizeleft -= E_SLOTSIZE;
				res++;
			}
		}
		if (sizeleft > 0)
			continue;
		
		if (type == ERT_ZORROII) {
			WriteExpansionByte(board, 18, startaddr >> 16);
		} else {
			WriteExpansionWord(board, 17, startaddr >> 16);
		}
		configDev->cd_BoardAddr	 = (APTR)startaddr;
		return TRUE;
	}

	configDev->cd_Flags |= CDF_SHUTUP;
	WriteExpansionByte(board, 19, 0); // SHUT-UP!
	return FALSE;

    AROS_LIBFUNC_EXIT
} /* ConfigBoard */

