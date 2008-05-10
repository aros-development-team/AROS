/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <strings.h>

#include "extstrings.h"
#include "afsblocks.h"

ULONG getHashKey(CONST_STRPTR name, ULONG tablesize, UBYTE flags) {
ULONG result;
UWORD length, i;

	length=strlen(name);
	if (length>MAX_NAME_LENGTH)
		length=MAX_NAME_LENGTH;
	result=length;
	for (i=0; i<length; i++)
		result=(result * 13 + capitalch(*name++,flags)) & 0x7FF;
	return result%tablesize;
}
