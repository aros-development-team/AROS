/*
    Copyright © 2003, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdlib.h>

#include <exec/types.h>

#include "support_aros.h"

/***************************************************
 Like StrToLong() but for hex numbers
 that represent addresses
***************************************************/
#ifdef __AROS__
LONG HexToIPTR(CONST_STRPTR s, IPTR *val)
{
    char *end;
    *val = (IPTR)strtoull(s,&end,16);
    if (end == (char*)s) return -1;
    return end - (char*)s;
}
LONG HexToLong(CONST_STRPTR s, ULONG *val)
{
    char *end;
    *val = (ULONG)strtoul(s,&end,16);
    if (end == (char*)s) return -1;
    return end - (char*)s;
}
#else
LONG HexToIPTR(CONST_STRPTR s, IPTR *val)
{
    return HexToLong((STRPTR)s, val);
}
#endif
