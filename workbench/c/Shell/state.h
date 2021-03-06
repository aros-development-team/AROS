/*
    Copyright (C) 2021, The AROS Development Team. All rights reserved.
*/

#ifndef SHELL_STATE_H
#define SHELL_STATE_H 1

#include <dos/dosextens.h>
#include <dos/dos.h>	/* for BPTR		*/

#define FILE_MAX 256 /* max length of file name */
#define LINE_MAX 512 /* max length of full command line */

#define MAXARGS    32
#define MAXARGLEN  32

struct SArg
{
    TEXT   name[MAXARGLEN];
    LONG   namelen;
    LONG   len;
    IPTR   def;
    LONG   deflen;
    UBYTE  type;
};

typedef struct _ShellState
{
    BPTR	newIn;
    BPTR	newOut;
    BPTR	oldIn;
    BPTR	oldOut;

    TEXT	command[FILE_MAX + 2];	/* command buffer */

    BPTR	oldHomeDir;	/* shared lock on program file's directory */

    LONG	cliNumber;

    LONG	argcount;	/* script args count */
    struct SArg	args[MAXARGS];	/* args definitions */
    IPTR	arg[MAXARGS];	/* args values */
    struct RDArgs *arg_rd;	/* Current RDArgs return state */

    TEXT	bra, ket, dollar, dot;
    TEXT        mchar0, pchar0;

    struct _ShellState *stack;

    ULONG	flags;		/* DOS/CliInit*() flags cache */

    APTR        ss_DOSBase;
    APTR        ss_SysBase;
} ShellState;

#define DOSBase (ss->ss_DOSBase)
#define SysBase ((struct ExecBase *)ss->ss_SysBase)

#endif
