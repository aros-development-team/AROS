/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#define DEBUG 1
#include <aros/debug.h>
#include "expansion_intern.h"
#include <proto/expansion.h>
#include <aros/asmcall.h>

#define Z3SLOT 0x01000000

/* do not touch. Ugly hack. UAE direct JIT versions need this */
/* check UAE expansion.c for ugly details */
AROS_UFH5(void, writeexpansion,
    AROS_UFHA(APTR, board, A0),
    AROS_UFHA(APTR, configdev, A3), // <- configdev = A3. This is important.
    AROS_UFHA(UBYTE, type, D0),
    AROS_UFHA(UWORD, startaddr, D1),
    AROS_UFHA(struct ExpansionBase *, ExpansionBase, A6))
{ 
    AROS_USERFUNC_INIT

    if (type == ERT_ZORROII) {
	WriteExpansionByte(board, 18, startaddr);
    } else {
	WriteExpansionWord(board, 17, startaddr);
    }

    AROS_USERFUNC_EXIT
}

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
	BOOL memorydevice;
	ULONG size = configDev->cd_BoardSize;
	
	D(bug("Configuring board: mfg=%d prod=%d size=%08x type=%02x\n",
	    configDev->cd_Rom.er_Manufacturer, configDev->cd_Rom.er_Product, size, configDev->cd_Rom.er_Type));

	memorydevice = (configDev->cd_Rom.er_Type & ERTF_MEMLIST) != 0;
	if (type == ERT_ZORROIII) {
		UWORD prevslot;
		UWORD endslot = 255;
		UWORD slotsize = (size + 0x00ffffff) / Z3SLOT;
		if (IntExpBase(ExpansionBase)->eb_z3Slot == 0)
			IntExpBase(ExpansionBase)->eb_z3Slot = 0x40000000 / Z3SLOT;
		prevslot = IntExpBase(ExpansionBase)->eb_z3Slot;
		D(bug("size=%d prev=%d end=%d\n", slotsize, prevslot, endslot));
		if (prevslot + slotsize <= endslot) {
			ULONG startaddr = prevslot * Z3SLOT;
			IntExpBase(ExpansionBase)->eb_z3Slot += slotsize;
			configDev->cd_BoardAddr	 = (APTR)startaddr;
			AROS_UFC5(void, writeexpansion,
				AROS_UFCA(APTR,  board, A0),
				AROS_UFCA(APTR,  configDev, A3),
				AROS_UFCA(UBYTE, type, D0),
	                	AROS_UFCA(UWORD, (startaddr >> 16), D1),
	                       	AROS_UFCA(struct ExpansionBase*, ExpansionBase, A6)
	             	);
			return TRUE;
		}
	} else {
		ULONG start, end, addr, align;
		UBYTE *space;
		start = 0x00200000;
		end   = 0x009FFFFF;
		space = IntExpBase(ExpansionBase)->eb_z2Slots;
		align = configDev->cd_BoardSize;
		if (!memorydevice) {
			start = 0x00E90000;
			end   = 0x00EFFFFF;
			align = size;
			/* Blizzard 1240/1260 SCSI kit (128k rom) must be at 0x00EA0000
		 	* Does this mean all >64k boards must be 128k aligned? */
			if (size > E_SLOTSIZE)
				start = 0x00EA0000;
		}
		for (addr = start; addr < end; addr += align) {
			ULONG startaddr = addr;
			UWORD offset = startaddr / (E_SLOTSIZE * SLOTSPERBYTE);
			BYTE bit = 7 - ((startaddr / E_SLOTSIZE) % SLOTSPERBYTE);
			UBYTE res = space[offset];
			ULONG sizeleft = size;
	
			if (res & (1 << bit))
				continue;

			// found free start address
			if (size >= E_SLOTSIZE * SLOTSPERBYTE) {
				// needs at least 1 byte and is always aligned to byte
				while (space[offset] == 0 && sizeleft >= E_SLOTSIZE && offset <= end / (E_SLOTSIZE * SLOTSPERBYTE)) {
					offset++;
					sizeleft -= E_SLOTSIZE * SLOTSPERBYTE;
				}
			} else {
				// bit by bit small board check (fits in one byte)
				while ((res & (1 << bit)) == 0 && sizeleft >= E_SLOTSIZE && bit >= 0) {
					sizeleft -= E_SLOTSIZE;
					bit--;
				}
			}

			if (sizeleft >= E_SLOTSIZE)
				continue;

			configDev->cd_BoardAddr	 = (APTR)startaddr;
			AROS_UFC5(void, writeexpansion,
				AROS_UFCA(APTR,  board, A0),
				AROS_UFCA(APTR,  configDev, A3),
				AROS_UFCA(UBYTE, type, D0),
	                	AROS_UFCA(UWORD, (startaddr >> 16), D1),
	                       	AROS_UFCA(struct ExpansionBase*, ExpansionBase, A6)
	             	);
		
			// do not remove this, configDev->cd_BoardAddr
			// might have changed inside writeexpansion
			startaddr = (ULONG)configDev->cd_BoardAddr;
			offset = startaddr / (E_SLOTSIZE * SLOTSPERBYTE);
			bit = 7 - ((startaddr / E_SLOTSIZE) % SLOTSPERBYTE);
			sizeleft = size;
			// now allocate area we reserved
			while (sizeleft >= E_SLOTSIZE) {
				space[offset] |= 1 << bit;
				sizeleft -= E_SLOTSIZE;
				bit--;
			}
	
			return TRUE;
		}
	}

	D(bug("Configuration failed!\n"));
	if (!(configDev->cd_Flags & ERFF_NOSHUTUP)) {
		configDev->cd_Flags |= CDF_SHUTUP;
		WriteExpansionByte(board, 19, 0); // SHUT-UP!
	} else {
		// uh?
	}
	return FALSE;

    AROS_LIBFUNC_EXIT
} /* ConfigBoard */

