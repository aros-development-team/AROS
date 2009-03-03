/*
    Copyright © 2005-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/memory.h>
#include <dos/var.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/muimaster.h>

#include <ctype.h>

#include "locale.h"
#include "support.h"

#include <stdio.h>

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
        application, window, 0, _(MSG_TITLE), _(MSG_ERROR_OK), 
        "%s:\n%s%s%s%s", _(MSG_ERROR_HEADER), message, newline, extra, period
    );
}

ULONG FormatSize(STRPTR buffer, ULONG blocks, ULONG totalblocks, ULONG bytesperblock, BOOL showPercentage)
{
    static STRPTR suffixes[] = {" bytes", "K", "M", "G", "T", "P"};
    DOUBLE internalsize = (DOUBLE)((UQUAD)blocks * bytesperblock);
    ULONG divcount = 0;
    ULONG percentage;

    if (totalblocks != 0)
        percentage = 100 * blocks / totalblocks;
    else
        percentage = 100;

    while (internalsize > 1024)
    {
        internalsize /= 1024;
        divcount++;
    }
    
    if (!showPercentage)
        sprintf(buffer, "%.1f%s  (%d %s)", internalsize, suffixes[divcount], blocks, _(MSG_BLOCKS) );
    else
        sprintf(buffer, "%.1f%s  (%d %s, %d%%)", internalsize, suffixes[divcount], blocks, _(MSG_BLOCKS), percentage);
    
    return percentage;
}
