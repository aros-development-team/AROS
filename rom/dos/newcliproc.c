#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "dos_newcliproc.h"

AROS_UFH3(LONG, NewCliProc,
AROS_UFHA(char *,argstr,A0),
AROS_UFHA(ULONG,argsize,D0),
AROS_UFHA(struct ExecBase *,SysBase,A6))
{
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

        rc = RunCommand(ShellSeg, cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT, argstr, argsize);

        CloseLibrary(DOSBase);
    }

    if (!Asynch)
    {
        csm->csm_ReturnCode = rc;
	ReplyMsg((struct Message *)csm);
    }

    UnLoadSeg(ShellSeg);

    return rc;
}
