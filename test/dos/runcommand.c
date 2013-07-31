#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/bptr.h>
#include <dos/dos.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int lastresult = RETURN_OK;
struct Task *parent;
LONG parent_signal;

static LONG get_default_stack_size()
{
    struct CommandLineInterface *cli = Cli();
    return cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT;
}

LONG launcher(void)
{
    char *cmd="C:Echo", *args = "RunCommand In Child";
    BPTR seglist;

    FPuts(Output(), "Child calls Echo\n");
    seglist = LoadSeg(cmd);
    if (seglist != BNULL)
    {
        SetProgramName(cmd);
        lastresult = RunCommand(
            seglist,get_default_stack_size(),args,strlen(args)
        );
        UnLoadSeg(seglist);
    }
    else
        FPuts(Output(), "Loading of Echo failed\n");
    Signal(parent, 1 << parent_signal);

    return 0;
}

int main(int argc, char **argv)
{
    char *fname = "SYS:Utilities/Clock";
    char *full = "";
    
    if(fname) {
        BPTR seglist = LoadSeg(fname);
        if(seglist)
        {
            SetProgramName(fname);
            lastresult=RunCommand(seglist,get_default_stack_size(),
                    full,strlen(full));
            UnLoadSeg(seglist);
        }
    }

    if (lastresult != RETURN_OK)
        goto done;

    struct TagItem tags[] =
    {
        { NP_Entry,         (IPTR) launcher },
        { NP_CloseInput,    (IPTR) FALSE },
        { NP_CloseOutput,   (IPTR) FALSE },
        { NP_CloseError,    (IPTR) FALSE },
        { NP_Cli,           (IPTR) TRUE },
        { NP_Name,          (IPTR) "runcommand_child" },
        { NP_NotifyOnDeath, (IPTR) TRUE },
        { TAG_DONE, 0 }
    };
    parent = FindTask(NULL);
    parent_signal = AllocSignal(-1);
    if (parent_signal == -1)
    {
        FPuts(Output(), "Signal Allocation failed\n");
        lastresult = 20;
        goto done;
    }
    FPuts(Output(), "Starting child...\n");
    CreateNewProc(tags);
    Wait(1 << parent_signal);
    FreeSignal(parent_signal);
    FPuts(Output(), "Child finished\n");

 done:
    exit(lastresult);
}
