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
 (This function is available in OS4)
***************************************************/
LONG HexToLong(STRPTR s, ULONG *val)
{
    char *end;
    *val = strtoul(s,&end,16);
    if (end == (char*)s) return -1;
    return end - (char*)s;
}
