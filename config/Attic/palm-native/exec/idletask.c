/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <registers.h>

void idleTask(void)
{
	ULONG d;
#warning Let the idle task 'do' something better. Switch off display after a while?
	while (1) {
		WREG_B(LPOSR) = (RREG_B(LPOSR) - 1) & 0x0f;
		d = 0;
		while (d < 0x10000)
			d++;
	}
}
