/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/memory.h>
#include <dos/var.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/muimaster.h>

#include <ctype.h>

#include "support.h"
#include "locale.h"

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

VOID ShowError(Object *application, Object *window, CONST_STRPTR message, BOOL useIOError)
{
    TEXT   buffer[128];
    STRPTR newline = "\n",
           period  = ".",
           extra   = buffer;
           
    /* Never use IO error if it is 0 */
    if (IoErr() == 0) useIOError = FALSE;
    
    if (useIOError)
    {
        Fault(IoErr(), NULL, buffer, sizeof(buffer));
        buffer[0] = toupper(buffer[0]);
    }
    else
    {
        newline = "";
        period  = "";
        extra   = "";
    }
            
    MUI_Request
    (
        application, window, 0, "SFS Defrag", "Ok", 
        "%s:\n%s%s%s%s", "ERROR", message, newline, extra, period
    );
}
