/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

    BPTR ShellSeg, CurrentInput;
    BOOL Background, Asynch;

    me  = (struct Process *)FindTask(NULL);
    WaitPort(&me->pr_MsgPort);
    csm = (struct CliStartupMessage *)GetMsg(&me->pr_MsgPort);


    DOSBase = (struct DosLibrary *)OpenLibrary(DOSNAME, 39);

    ShellSeg     = csm->csm_ShellSeg;
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

	cli->cli_StandardInput  = Input();
        cli->cli_StandardOutput =
    	cli->cli_CurrentOutput  = Output();
	cli->cli_StandardError  = Error();
    	cli->cli_CurrentInput   = CurrentInput;
    	cli->cli_Interactive    = cli->cli_CurrentInput == cli->cli_StandardInput ? DOSTRUE : DOSFALSE;
    	cli->cli_Background     = Background;

	if (!Background)
	{
	    struct IOFileSys iofs;
	    struct FileHandle *fhin  = BADDR(Input());
	    struct FileHandle *fhout = BADDR(Output());

            iofs.IOFS.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
            iofs.IOFS.io_Message.mn_ReplyPort    = &me->pr_MsgPort;
            iofs.IOFS.io_Message.mn_Length       = sizeof(struct IOFileSys);
            iofs.IOFS.io_Command                 = FSA_CHANGE_SIGNAL;
            iofs.IOFS.io_Flags                   = 0;

	    iofs.io_Union.io_CHANGE_SIGNAL.io_Task = (struct Task *)me;

	    iofs.IOFS.io_Device  = fhin->fh_Device;
    	    iofs.IOFS.io_Unit    = fhin->fh_Unit;

	    DoIO(&iofs.IOFS);

	    iofs.IOFS.io_Device  = fhout->fh_Device;
    	    iofs.IOFS.io_Unit    = fhout->fh_Unit;

	    DoIO(&iofs.IOFS);
        }

	rc = RunCommand(ShellSeg, cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT, argstr, argsize);

        CloseLibrary((struct Library *)DOSBase);
    }

    if (!Asynch)
    {
        csm->csm_ReturnCode = Cli()->cli_ReturnCode;
	ReplyMsg((struct Message *)csm);
    }

    UnLoadSeg(ShellSeg);

    return rc;

    AROS_USERFUNC_EXIT
}
