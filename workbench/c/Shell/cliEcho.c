/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <proto/dos.h>
#include "Shell.h"

void cliEcho(ShellState *ss, CONST_STRPTR args)
{
    struct LocalVar *lv;
    BPTR echoOut;

    /* AmigaDOS's shell is content also with echo being set to anything
       that begins with "on" in order to trigger commands echoing on,
       it doesn't really have to be set to just "on". */
    if ((lv = FindVar("echo", LV_VAR)) == NULL)
	return;

    switch (lv->lv_Value[0])
    {
    case 'o':
    case 'O':
	switch (lv->lv_Value[1])
	{
	case 'n':
	case 'N':
	    break;
	default:
	    return;
	}
	break;
    default:
	return;
    }

    /* Ok, commands echoing is on. */
    /* If a redirection is present, echoing isn't expected to go to
       it. If a script is running, building commandLine allows us
       to show what the command line looks like after arguments
       substitution. */
    echoOut = ss->newOut ? ss->oldOut : Output();

    FPuts(echoOut, ss->command + 2);

    if (args)
    {
	FPutC(echoOut, ' ');
	FPuts(echoOut, args);
    }

    FPutC(echoOut, '\n');
}

