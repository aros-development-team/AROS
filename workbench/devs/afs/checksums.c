/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"
#include "checksums.h"

ULONG calcChkSum(ULONG SizeBlock, ULONG *buffer) {
ULONG sum=0,count=0;

	for (count=0;count<SizeBlock;count++)
		sum += OS_BE2LONG(buffer[count]);
	return sum;
}
