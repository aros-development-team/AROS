/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dostags.h>

#ifdef __AMIGA__
#define IPTR ULONG
#define CLI_DEFAULTSTACK_UNIT sizeof(IPTR)
typedef struct RAWARG_s { } *RAWARG;
#endif

/*

This is output of the test from AmigaOS 3.1 Shell
:> stack 50000
:> clicreatenewproc
Main process, stack: 3200, cli_stack 50000
Creating subprocess, no stack set, no cli
Subprocess, stack: 4000, cli_stack -1
Creating subprocess, stack set to 64000, no cli
Subprocess, stack: 64000, cli_stack -1
Creating subprocess, stack set to 64000, cli
Subprocess, stack: 64000, cli_stack 64000
Creating subprocess, no stack set, cli
Subprocess, stack: 4000, cli_stack 4000

This is output of the test from AROS
:> stack 50000
:> clicreatenewproc
Main process, stack: 262144, cli_stack 50000
Creating subprocess, no stack set, no cli
Subprocess, stack: 50000, cli_stack -1
Creating subprocess, stack set to 64000, no cli
Subprocess, stack: 64000, cli_stack -1
Creating subprocess, stack set to 64000, cli
Subprocess, stack: 64000, cli_stack 64000
Creating subprocess, no stack set, cli
Subprocess, stack: 50000, cli_stack 50000

On AROS, unless NP_StackStack size is provided, child processes inherit stack
from parent process cli_DefaultStack if available.

*/

static BYTE sig;
static struct Task *task;

static void __printf_info(struct Process *p, struct CommandLineInterface *cli)
{
    LONG argl[2];
    argl[0] = p->pr_StackSize;
    argl[1] = cli ? cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT: -1;

    VPrintf(", stack: %ld, cli_stack %ld\n", (RAWARG)argl);
}
static void SubProcess(void)
{
    struct CommandLineInterface *cli;
    struct Process *me;

    me = (struct Process *)FindTask(NULL);
    cli = Cli();
    VPrintf("Subprocess", NULL);__printf_info(me, cli);

    Signal(task, 1<<sig);
}

int main(void)
{
    struct CommandLineInterface *cli;
    struct Process *me;

    sig = AllocSignal(-1);
    task = FindTask(NULL);

    me = (struct Process *)task;
    cli = Cli();

    VPrintf("Main process", NULL);__printf_info(me, cli);

    VPrintf("Creating subprocess, no stack set, no cli\n", NULL);
    CreateNewProcTags(NP_Entry, (IPTR) SubProcess, NP_Output, Output(), NP_CloseOutput, FALSE);
    Wait(1<<sig);

    VPrintf("Creating subprocess, stack set to 64000, no cli\n", NULL);
    CreateNewProcTags(NP_Entry, (IPTR) SubProcess, NP_Output, Output(), NP_CloseOutput, FALSE,
        NP_StackSize, 64000);
    Wait(1<<sig);

    VPrintf("Creating subprocess, stack set to 64000, cli\n", NULL);
    CreateNewProcTags(NP_Entry, (IPTR) SubProcess, NP_Output, Output(), NP_CloseOutput, FALSE,
        NP_StackSize, 64000, NP_Cli, TRUE);
    Wait(1<<sig);

    VPrintf("Creating subprocess, no stack set, cli\n", NULL);
    CreateNewProcTags(NP_Entry, (IPTR) SubProcess, NP_Output, Output(), NP_CloseOutput, FALSE,
        NP_Cli, TRUE);
    Wait(1<<sig);

    FreeSignal(sig);

    return 0;
}
