/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <asm/registers.h>

void idleTask(void)
{
	while (1) {
//		*(BYTE *)0xdddddebc = '~';
	}
}
