/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>
#include <string.h>

#include "support.h"

BOOL ReadLine(BPTR fh, STRPTR buffer, ULONG size)
{
    if (FGets(fh, buffer, size) != NULL)
    {
        ULONG last = strlen(buffer) - 1;
        if (buffer[last] == '\n') buffer[last] = '\0';
        
        return TRUE;
    }
    
    return FALSE;
}
