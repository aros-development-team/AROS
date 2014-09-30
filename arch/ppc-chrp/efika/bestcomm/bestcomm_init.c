/*
    Copyright © 2009-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/openfirmware.h>
#include <proto/kernel.h>

#include "bestcomm_private.h"

static int bestcomm_init(struct BestCommBase *BestCommBase)
{
	int retval = TRUE;
	void *OpenFirmwareBase;
	void *key, *prop;

	D(bug("[SDMA] BestComm Init.\n"));

	OpenFirmwareBase = OpenResource("openfirmware.resource");

	/* Get some addresses from OF tree */
	if (OpenFirmwareBase)
	{
		key = OF_OpenKey("/builtin");
		if (key)
		{
			prop = OF_FindProperty(key, "reg");
			if (prop)
			{
				reg_t *reg = OF_GetPropValue(prop);
				BestCommBase->bc_MBAR = (uint8_t *)reg->addr;
			}
		}

		key = OF_OpenKey("/builtin/sram");
		if (key)
		{
			prop = OF_FindProperty(key, "reg");
			if (prop)
			{
				reg_t *reg = OF_GetPropValue(prop);
				BestCommBase->bc_SRAM = reg->addr;
				BestCommBase->bc_SRAMSize = reg->size;
			}
			D(bug("[SDMA] %d bytes SRAM at %08x\n", BestCommBase->bc_SRAMSize, BestCommBase->bc_SRAM));
			if (BestCommBase->bc_SRAMSize)
			{
				/* Get memory for Free bitmap. Here, 1=free and 0=in use */
				BestCommBase->bc_SRAMFree = AllocMem((BestCommBase->bc_SRAMSize >> 4) / (8*sizeof(uint32_t)), MEMF_CLEAR|MEMF_PUBLIC);

				prop = OF_FindProperty(key, "available");
				if (prop)
				{
					int i;
					const int regs = OF_GetPropLen(prop) / sizeof(reg_t);
					reg_t *reg = OF_GetPropValue(prop);

					for (i=0; i < regs; i++)
					{
						D(bug("[SDMA] addr=%08x len=%d\n", reg[i].addr, reg[i].size));


					}
				}
			}
		}

		key = OF_OpenKey("/builtin/bestcomm");
		if (key)
		{
			void *prop = OF_FindProperty(key, "reg");
			if (prop)
			{
				BestCommBase->bc_BestComm = *(void **)OF_GetPropValue(prop);
			}
			D(bug("[SDMA] bestcomm at %08x\n", BestCommBase->bc_BestComm));
		}
	}
	else
		retval = FALSE;



	return retval;
}


ADD2INITLIB(bestcomm_init, 0)
