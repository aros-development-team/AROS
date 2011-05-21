/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

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
	Close(ss->newIn);

    if (ss->newOut)
	Close(ss->newOut);

    ss->newIn = BNULL;
    ss->newOut = BNULL;
}
