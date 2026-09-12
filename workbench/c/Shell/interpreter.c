/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
 */

#include <proto/dos.h>
#include <proto/exec.h>

#include "Shell.h"

void initDefaultInterpreterState(ShellState *ss)
{
    ss->argcount = 0;
    ss->arguments = NULL;

    /* Preset the first bytes to "C:" to handle C: multiassigns */
    ss->command[0] = 'C';
    ss->command[1] = ':';

    ss->arg_rd = NULL;
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

void freeInterpreterState(ShellState *ss)
{
    struct SArg *a;
    LONG i;

    for (i = 0; ss->arguments && i < ss->argcount; ++i)
    {
        a = ss->arguments->args + i;

        if (a->def)
            FreeMem((APTR) a->def, a->deflen + 1);
    }

    if (ss->arg_rd)
        FreeDosObject(DOS_RDARGS, ss->arg_rd);

    if (ss->arguments)
        FreeMem(ss->arguments, sizeof(*ss->arguments));

    ss->arg_rd = NULL;
    ss->arguments = NULL;
    ss->argcount = 0;
}

void popInterpreterState(ShellState *ss)
{
    ShellState *tmp_ss = ss->stack;

    freeInterpreterState(ss);

    if (tmp_ss)
    {
        *ss = *tmp_ss;
        FreeMem(tmp_ss, sizeof(*ss));
    }
    else
        initDefaultInterpreterState(ss);
}
