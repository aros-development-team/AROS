/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/memory.h>
#include <dos/var.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "support.h"

STRPTR GetENV(CONST_STRPTR name)
{
    UBYTE  dummy = 0;
    STRPTR value = NULL;
    
    /* Check that the variable exists, and get the length */
    if (GetVar(name, &dummy, 1, GVF_GLOBAL_ONLY) != -1)
    {
        ULONG length = IoErr() + 1;
        
        if ((value = AllocVec(length, MEMF_ANY)) != NULL)
        {
            if (GetVar(name, value, length, GVF_GLOBAL_ONLY) == -1)
            {
                FreeVec(value);
                value = NULL;
            }
        }
    }
    
    return value;
}

BOOL SetENV(CONST_STRPTR name, CONST_STRPTR value)
{
    return SetVar(name, value, -1, GVF_GLOBAL_ONLY);
}
