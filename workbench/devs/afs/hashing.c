/* 
   $Id$
*/

#include <strings.h>

#include "extstrings.h"

ULONG getHashKey(STRPTR name,ULONG tablesize, UBYTE flags) {
ULONG result;

	result=StrLen(name);
	while (*name!=0)
		result=(result * 13 +capitalch(*name++,flags)) & 0x7FF;
	return result%tablesize;
}
