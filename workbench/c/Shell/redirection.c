/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
 */

#include <aros/debug.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "Shell.h"

static BPTR selectErrorOutput(ShellState *ss, BPTR fh)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    BPTR old = me->pr_CES;

    me->pr_CES = fh;
    return old;
}

LONG Redirection_init(ShellState *ss)
{
    ss->newErr = BNULL;
    ss->oldErr = BNULL;
    ss->oldConsoleTask = NULL;
    ss->errorFile[0] = '\0';
    ss->errorRedirect = FALSE;
    ss->errorAppend = FALSE;
    ss->errorToOutput = FALSE;
    ss->errorActive = FALSE;
    ss->consoleTaskChanged = FALSE;

    return 0;
}

LONG Redirection_activateError(ShellState *ss)
{
    BPTR err;

    if (!ss->errorRedirect || ss->errorActive)
        return 0;

    if (ss->errorToOutput)
    {
        err = Output();
    }
    else
    {
        LONG mode = ss->errorAppend ? MODE_READWRITE : MODE_NEWFILE;

        ss->newErr = Open(ss->errorFile, mode);
        if (!ss->newErr)
            return IoErr();

        if (ss->errorAppend && Seek(ss->newErr, 0, OFFSET_END) == -1)
        {
            LONG error = IoErr();

            Close(ss->newErr);
            ss->newErr = BNULL;
            SetIoErr(error);
            return error;
        }

        err = ss->newErr;
    }

    ss->oldErr = selectErrorOutput(ss, err);
    ss->errorActive = TRUE;

    if (ss->newErr && IsInteractive(ss->newErr))
    {
        struct FileHandle *fh = BADDR(ss->newErr);

        ss->oldConsoleTask = SetConsoleTask(fh->fh_Type);
        ss->consoleTaskChanged = TRUE;
    }

    return 0;
}

void Redirection_release(ShellState *ss)
{
    if (ss->errorActive)
    {
        selectErrorOutput(ss, ss->oldErr);
        ss->errorActive = FALSE;
    }

    if (ss->consoleTaskChanged)
    {
        SetConsoleTask(ss->oldConsoleTask);
        ss->consoleTaskChanged = FALSE;
    }

    if (ss->newErr)
    {
        D(bug("[Shell] Closing redirected error output 0x%p, old 0x%p\n",
              ss->newErr, ss->oldErr));
        Close(ss->newErr);
    }

    ss->newErr = BNULL;
    ss->oldErr = BNULL;
    ss->oldConsoleTask = NULL;
    ss->errorFile[0] = '\0';
    ss->errorRedirect = FALSE;
    ss->errorAppend = FALSE;
    ss->errorToOutput = FALSE;

    /* Close redirection files and install regular input and output streams */
    if (ss->newIn)
    {
        D(bug("[Shell] Closing redirected input 0x%p, old 0x%p\n", ss->newIn, ss->oldIn));
        SelectInput(ss->oldIn);
        Close(ss->newIn);
    }

    if (ss->newOut)
    {
        D(bug("[Shell] Closing redirected output 0x%p, old 0x%p\n", ss->newOut, ss->oldOut));
        SelectOutput(ss->oldOut);
        Close(ss->newOut);
    }

    ss->newIn = BNULL;
    ss->newOut = BNULL;
}
