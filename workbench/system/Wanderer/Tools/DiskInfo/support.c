/*
    Copyright (C) 2005-2023, The AROS Development Team. All rights reserved.
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

static CONST_STRPTR suffixes[] = {"K", "M", "G", "T", "P"};

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

VOID FormatSize(STRPTR buffer, ULONG bufsize, ULONG size)
{
    CONST_STRPTR suffix = _(MSG_BYTES);
    ULONG divcount = 0;

    while (size > 1024)
    {
        size /= 1024;
        suffix = suffixes[divcount];
        divcount++;
    }
    
    snprintf(buffer, bufsize, "%u%s", size, suffix);
}

ULONG FormatBlocksSized(STRPTR buffer, ULONG bufsize, ULONG blocks, ULONG totalblocks, ULONG bytesperblock, BOOL showPercentage)
{
    DOUBLE internalsize = (DOUBLE)((UQUAD)blocks * bytesperblock);
    CONST_STRPTR suffix = _(MSG_BYTES);
    ULONG divcount = 0;
    ULONG percentage;

    if (totalblocks != 0)
        percentage = 100 * blocks / totalblocks;
    else
        percentage = 100;

    while (internalsize > 1024)
    {
        internalsize /= 1024;
        suffix = suffixes[divcount];
        divcount++;
    }
    
    if (!showPercentage)
        snprintf(buffer, bufsize, "%.1f%s  (%d %s)", internalsize, suffix, (int)blocks, _(MSG_BLOCKS) );
    else
        snprintf(buffer, bufsize, "%.1f%s  (%d %s, %d%%)", internalsize, suffix, (int)blocks, _(MSG_BLOCKS), (int)percentage);
    
    return percentage;
}
