/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "dos_newcliproc.h"
#include "fs_driver.h"

AROS_UFH2(LONG, NewCliProc,
AROS_UFHA(char *,argstr,A0),
AROS_UFHA(ULONG,argsize,D0))
{
    AROS_USERFUNC_INIT
    
    struct Process *me;
    struct CliStartupMessage *csm;
    LONG rc = RETURN_FAIL;
    struct DosLibrary *DOSBase;

    BPTR CurrentInput;
    BOOL Background, Asynch;

    me  = (struct Process *)FindTask(NULL);
    WaitPort(&me->pr_MsgPort);
    csm = (struct CliStartupMessage *)GetMsg(&me->pr_MsgPort);

    DOSBase = (struct DosLibrary *)OpenLibrary(DOSNAME, 39);

    CurrentInput = csm->csm_CurrentInput;
    Background   = csm->csm_Background;
    Asynch       = csm->csm_Asynch;

    csm->csm_CliNumber = me->pr_TaskNum;

    if (Asynch)
    {
        csm->csm_ReturnCode = DOSBase ? RETURN_OK : RETURN_FAIL;
	ReplyMsg((struct Message *)csm);
    }

    if (DOSBase)
    {
	struct CommandLineInterface *cli = Cli();
	BPTR *ShellSeg = BADDR(me->pr_SegList);

	cli->cli_StandardInput  = Input();
        cli->cli_StandardOutput =
    	cli->cli_CurrentOutput  = Output();
	cli->cli_StandardError  = me->pr_CES;
    	cli->cli_CurrentInput   = CurrentInput;
    	cli->cli_Interactive    = cli->cli_CurrentInput == cli->cli_StandardInput ? DOSTRUE : DOSFALSE;
    	cli->cli_Background     = Background;

	if (!Background)
	{
	    BPTR fhin  = Input();
	    BPTR fhout = Output();

	    if (fhin != BNULL)
            	fs_ChangeSignal(fhin, me, DOSBase);
            if (fhout != BNULL)
            	fs_ChangeSignal(fhout, me, DOSBase);
        }

	rc = RunCommand(ShellSeg[3], cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT, argstr, argsize);

        CloseLibrary((struct Library *)DOSBase);
    }

    if (!Asynch)
    {
        csm->csm_ReturnCode = Cli()->cli_ReturnCode;
	ReplyMsg((struct Message *)csm);
    }

    return rc;

    AROS_USERFUNC_EXIT
}
