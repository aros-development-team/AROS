/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <proto/exec.h>

#include "Shell.h"

void initDefaultInterpreterState(ShellState *ss)
{
    LONG i;

    ss->argcount = 0;

    /* Preset the first bytes to "C:" to handle C: multiassigns */
    ss->command[0] = 'C';
    ss->command[1] = ':';

    for (i =  0; i < MAXARGS; ++i)
    {
	struct SArg *a = ss->args + i;

	a->namelen  = 0;
	a->name[0]  = '\0';
	a->len      = 0;
	a->def      = 0;
	a->deflen   = 0;
	a->type     = NORMAL;

	ss->arg[i]  = 0;
    }

    ss->bra    = '<';
    ss->ket    = '>';
    ss->dollar = '$';
    ss->dot    = '.';

    ss->stack  = NULL;
}

LONG pushInterpreterState(ShellState *ss)
{
    ShellState *tmp_ss = (ShellState *)AllocMem(sizeof(*ss), MEMF_LOCAL);

    if (tmp_ss)
    {
	*tmp_ss = *ss;
	initDefaultInterpreterState(ss);
	ss->stack = tmp_ss;
	return 0;
    }

    return ERROR_NO_FREE_STORE;
}

void popInterpreterState(ShellState *ss)
{
    ShellState *tmp_ss = ss->stack;
    struct SArg *a;
    LONG i;

    for (i = 0; i < ss->argcount; ++i)
    {
	a = ss->args + i;

	if (a->def)
	    FreeMem((APTR) a->def, a->deflen + 1);
    }

    if (tmp_ss)
    {
	*ss = *tmp_ss;
	FreeMem(tmp_ss, sizeof(*ss));
    }
    else
	initDefaultInterpreterState(ss);
}
