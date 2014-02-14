/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#define DEBUG 1
#include <aros/debug.h>
#include "expansion_intern.h"
#include <proto/expansion.h>
#include <aros/asmcall.h>

/* See rom/expansion/configboard.c for documentation */

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


#include <clib/expansion_protos.h>

AROS_LH2(BOOL, ConfigBoard,
    AROS_LHA(APTR              , board, A0),
    AROS_LHA(struct ConfigDev *, configDev, A1),
    struct ExpansionBase *, ExpansionBase, 10, Expansion)
{
    AROS_LIBFUNC_INIT

	UBYTE type = configDev->cd_Rom.er_Type & ERT_TYPEMASK;
	BOOL memorydevice;
	ULONG size = configDev->cd_BoardSize;
	
	D(bug("Configuring board: cd=%p mfg=%d prod=%d size=%08x type=%02x\n",
	    configDev, configDev->cd_Rom.er_Manufacturer, configDev->cd_Rom.er_Product, size, configDev->cd_Rom.er_Type));

	memorydevice = (configDev->cd_Rom.er_Type & ERTF_MEMLIST) != 0;
	if (type == ERT_ZORROIII) {
		UWORD prevslot, newslot;
		UWORD endslot = 255;
		UWORD slotsize = (size + 0x00ffffff) / Z3SLOT;
		if (IntExpBase(ExpansionBase)->z3Slot == 0)
			IntExpBase(ExpansionBase)->z3Slot = 0x40000000 / Z3SLOT;
		prevslot = IntExpBase(ExpansionBase)->z3Slot;
		// handle alignment
		newslot = (prevslot + slotsize - 1) & ~(slotsize - 1);
		D(bug("size=%d prev=%d new=%d end=%d\n", slotsize, prevslot, newslot, endslot));
		if (newslot + slotsize <= endslot) {
			ULONG startaddr = newslot * Z3SLOT;
			configDev->cd_BoardAddr = (APTR)startaddr;
			configDev->cd_SlotAddr = IntExpBase(ExpansionBase)->z3Slot;
			configDev->cd_SlotSize = slotsize;
			configDev->cd_Flags |= CDF_CONFIGME;
			IntExpBase(ExpansionBase)->z3Slot = newslot + slotsize;
			AROS_UFC5NR(void, writeexpansion,
				AROS_UFCA(APTR,  board, A0),
				AROS_UFCA(APTR,  configDev, A3),
				AROS_UFCA(UBYTE, type, D0),
	                	AROS_UFCA(UWORD, (startaddr >> 16), D1),
	                       	AROS_UFCA(struct ExpansionBase*, ExpansionBase, A6)
	             	);
	             	D(bug("-> configured, %p - %p\n", startaddr, startaddr + configDev->cd_BoardSize - 1));
			return TRUE;
		}
	} else {
		ULONG start, end, addr, step;
		UBYTE *space;
		UBYTE area;
		
		for (area = 0; area < 2; area++) {
			
			if (area == 0 && (size >= 8 * E_SLOTSIZE || memorydevice))
				continue;
			
			if (area == 0) {
				start = 0x00E90000;
				end   = 0x00EFFFFF;
			} else {
				start = 0x00200000;
				end   = 0x009FFFFF;
			}
			space = IntExpBase(ExpansionBase)->z2Slots;
			step = 0x00010000;
			for (addr = start; addr < end; addr += step) {
				ULONG startaddr = addr;
				UWORD offset = startaddr / (E_SLOTSIZE * SLOTSPERBYTE);
				BYTE bit = 7 - ((startaddr / E_SLOTSIZE) % SLOTSPERBYTE);
				UBYTE res = space[offset];
				ULONG sizeleft = size;
		
				if (res & (1 << bit))
					continue;
	
				if (size < 4 * 1024 * 1024) {
					// handle alignment, 128k boards must be 128k aligned and so on..
					if ((startaddr & (size - 1)) != 0)
						continue;
				} else {
					// 4M and 8M boards have different alignment requirements
					if (startaddr != 0x00200000 && startaddr != 0x00600000)
						continue;
				}
	
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
	
				configDev->cd_BoardAddr = (APTR)startaddr;
				configDev->cd_Flags |= CDF_CONFIGME;
				configDev->cd_SlotAddr = (startaddr >> 16);
				configDev->cd_SlotSize = size >> 16;
				AROS_UFC5NR(void, writeexpansion,
					AROS_UFCA(APTR,  board, A0),
					AROS_UFCA(APTR,  configDev, A3),
					AROS_UFCA(UBYTE, type, D0),
		                	AROS_UFCA(UWORD, (startaddr >> 16), D1),
		                       	AROS_UFCA(struct ExpansionBase*, ExpansionBase, A6)
		             	);
		             	D(bug("-> configured, %p - %p\n", startaddr, startaddr + configDev->cd_BoardSize - 1));
			
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

