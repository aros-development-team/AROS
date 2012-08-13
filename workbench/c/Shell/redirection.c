/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <aros/debug.h>
#include <proto/dos.h>

#include "Shell.h"

LONG Redirection_init(ShellState *ss)
{
    return 0;
}

void Redirection_release(ShellState *ss)
{
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
