#include <aros/macros.h>

#include "checksums.h"

ULONG calcChkSum(ULONG SizeBlock, ULONG *buffer) {
ULONG sum=0,count=0;

	for (count=0;count<SizeBlock;count++)
		sum += AROS_BE2LONG(buffer[count]);
	return sum;
}
