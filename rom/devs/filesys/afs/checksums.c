/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"
#include "checksums.h"

ULONG calcChkSum(ULONG SizeBlock, ULONG *buffer) {
ULONG sum=0, value;
ULONG *p, *end;

	p = buffer;
	end = buffer + SizeBlock;
	while (p < end)
	{
		value = *p++;
		sum += OS_BE2LONG(value);
	}
	return sum;
}

