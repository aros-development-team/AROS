/*
    Copyright � 2013-2014, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: 
    Lang: English              
 */

/******************************************************************************


    NAME

        SymbolDump

    SYNOPSIS

        (N/A)

    LOCATION

        C:

    FUNCTION

         Dumps debug symbols to the file "SYS:symbols.out" to be used with
         Callgrind. See "Debugging manual" at www.aros.org.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <utility/hooks.h>
#include <libraries/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/debug.h>

const TEXT version[] = "$VER: SymbolDump 1.1 (18.5.2014)\n";

struct Library * DebugBase = NULL;

AROS_UFH3(static void, symbolhandler,
        AROS_UFHA(struct Hook *,        hook, A0),
        AROS_UFHA(APTR,                 object, A2),
        AROS_UFHA(struct SymbolInfo *,  message, A1))
{
    AROS_USERFUNC_INIT

    FPrintf(hook->h_Data, "S|%s|%s|0x%p|0x%p\n", message->si_ModuleName, message->si_SymbolName,
            message->si_SymbolStart, message->si_SymbolEnd);

    AROS_USERFUNC_EXIT
}

static void OpenLibraries()
{
    DebugBase = OpenLibrary("debug.library", 0L);
}

static void CloseLibraries()
{
    CloseLibrary(DebugBase);
    DebugBase = NULL;
}

int main(void)
{
    BPTR output = BNULL;

    OpenLibraries();

    output = Open("SYS:symbols.out", MODE_NEWFILE);

    if (output != BNULL)
    {
        struct Hook handler;
        handler.h_Entry = (HOOKFUNC)symbolhandler;
        handler.h_Data = (APTR)output;

        EnumerateSymbolsA(&handler, NULL);

        Close(output);
    }

    CloseLibraries();

    return 0;
}
