/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <registers.h>

void idleTask(void)
{
#warning Let the idle task 'do' something better. Switch off display after a while?
	ULONG i;


	while (1) {
		i = 0;
		while (i < 160/2) {
			drawlinehoriz(160-i);
			drawlinevert(160-i);
			i++;
		}
	}

}
