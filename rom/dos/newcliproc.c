/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <dos/dos.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "dos_newcliproc.h"
#include "dos_dosdoio.h"
#include "fs_driver.h"

AROS_UFH3(LONG, NewCliProc,
AROS_UFHA(char *,argstr,A0),
AROS_UFHA(ULONG,argsize,D0),
AROS_UFHA(struct ExecBase *,SysBase,A6))
{
    AROS_USERFUNC_INIT
    
    struct Process *me;
    struct CliStartupMessage *csm;
    LONG rc = RETURN_FAIL;
    struct DosLibrary *DOSBase;
    STRPTR alloced_argstr = NULL;

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
	cli->cli_StandardError  = Error();
    	cli->cli_CurrentInput   = CurrentInput;
    	cli->cli_Interactive    = cli->cli_CurrentInput == cli->cli_StandardInput ? DOSTRUE : DOSFALSE;
    	cli->cli_Background     = Background;

	if (!Background)
	{
	    struct FileHandle *fhin  = BADDR(Input());
	    struct FileHandle *fhout = BADDR(Output());

	    if (fhin)
            	fs_ChangeSignal(fhin, me, DOSBase);
            if (fhout)
            	fs_ChangeSignal(fhin, me, DOSBase);
        }

        /* If argstr is missing a newline terminator, add it */
        if (argsize == 0 || argstr[argsize-1] != '\n')
        {
            alloced_argstr = AllocVec(argsize+2, MEMF_ANY);
            if (alloced_argstr == NULL)
            {
            	rc = DOSFALSE;
            	goto exit;
            }
            CopyMem(argstr, alloced_argstr, argsize);
            alloced_argstr[argsize++]='\n';
            alloced_argstr[argsize]  = 0;
            argstr = alloced_argstr;
        }

	rc = RunCommand(ShellSeg[3], cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT, argstr, argsize);

	if (alloced_argstr != NULL)
	    FreeVec(alloced_argstr);

exit:
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
