/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/memory.h>
#include <proto/exec.h>
#include <inttypes.h>

#include "intelG45_intern.h"
#include "intelG45_regs.h"

intptr_t G45_VirtualToPhysical(struct g45staticdata *sd, intptr_t virtual)
{
	intptr_t page = virtual >> 12;	/* get page number */
	intptr_t result = -1;

	if (page >= 0 && page < sd->Card.GATT_size / 4)
	{
		uint32_t pte = readl(&sd->Card.GATT[page]);
		if (pte & 1)
		{
			result = pte & 0xfffff000;
			result |= virtual & 0xfff;
		}
	}

	return result;
}

void G45_AttachMemory(struct g45staticdata *sd, intptr_t physical, intptr_t virtual, intptr_t length)
{
	intptr_t page = virtual >> 12;

	if (page > 0)
	{
		physical &= 0xfffff000;
		length &= 0xfffff000;

		do {
			writel(physical | 1, &sd->Card.GATT[page]);

			physical += 4096;
			length -= 4096;
			page++;
		} while((page < sd->Card.GATT_size / 4) && length);
	}
    CacheClearU();
}

void G45_AttachCacheableMemory(struct g45staticdata *sd, intptr_t physical, intptr_t virtual, intptr_t length)
{
	intptr_t page = virtual >> 12;

	if (page > 0)
	{
		physical &= 0xfffff000;
		length &= 0xfffff000;

		do {
			writel(physical | 7, &sd->Card.GATT[page]);

			physical += 4096;
			length -= 4096;
			page++;
		} while((page < sd->Card.GATT_size / 4) && length);
	}
    CacheClearU();
}

void G45_DetachMemory(struct g45staticdata *sd, intptr_t virtual, intptr_t length)
{
	intptr_t page = virtual >> 12;

	if (page >= 0) do
	{
		writel(0, &sd->Card.GATT[page]);

		page++;
		length--;
	} while(length && page < (sd->Card.GATT_size / 4));
    CacheClearU();
}
