/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <registers.h>

void idleTask(void)
{
#warning Let the idle task 'do' something better. Switch off display after a while?
#if 0
	ULONG d;
	while (1) {

		WREG_B(LPOSR) = (RREG_B(LPOSR) - 1) & 0x0f;
		d = 0;
		while (d < 0x10000)
			d++;
	}
#endif
	ULONG i = 0;
	ULONG z = 0;
*(ULONG *)0x1=0x1;
	while (i < 160) {
		_drawlinehoriz(i);
		z = 0;
		while (z < 0x4000) {
			z++;
		}
		i++;
	}

}
