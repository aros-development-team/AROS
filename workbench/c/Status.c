/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Status CLI command
    Lang: english
*/

/*****************************************************************************

    NAME

        Status

    SYNOPSIS

        PROCESS/N,FULL/S,TCB/S,CLI=ALL/S,COM=COMMAND/K

    LOCATION

        Workbench:c

    FUNCTION

        Display information about the processes that are executing within Shells/CLIs.

    INPUTS

        PROCESS     - Process Identification number.

        FULL        - Display all information about the processes.

        TCB         - As for Full, except that this option omits the process name.

        CLI=ALL     - Default. Displays all processes.

        COM=COMMAND - Show the process id of the command given. Specify the command name.

    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

        Status

            Process  2: Loaded as command: c:status
            Process  3: Loaded as command: c:NewIcons
            Process  4: Loaded as command: GG:Sys/L/fifo-handler
            Process  5: Loaded as command: Workbench
            Process  6: Loaded as command: ToolsDaemon

        Status full

            Process  2: stk 300000, gv 150, pri   0 Loaded as command: c:status
            Process  3: stk  4096, gv 150, pri   0 Loaded as command: c:NewIcons
            Process  4: stk  4096, gv 150, pri   0 Loaded as command: GG:Sys/L/fifo-handler
            Process  5: stk  6000, gv 150, pri   1 Loaded as command: Workbench
            Process  6: stk  4000, gv 150, pri   2 Loaded as command: ToolsDaemon

    BUGS

    SEE ALSO

        dos/dosextens.h

    INTERNALS

    HISTORY

******************************************************************************/

#include <proto/dos.h>

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <exec/types.h>

#include <strings.h>
#include <stdio.h>

#define ARG_TEMPLATE    "PROCESS/N,FULL/S,TCB/S,CLI=ALL/S,COM=COMMAND/K"
#define ARG_PROCESS     0
#define ARG_FULL        1
#define ARG_TCB         2
#define ARG_ALL         3
#define ARG_COM         4
#define TOTAL_ARGS      5

#define NOT_SET         0

static const char version[] = "$VER: Status 41.0 (21.09.1997)\n";

int Do_Status(LONG *, LONG, LONG, LONG, STRPTR *);

int main(int argc, char *argv[])
{
    struct RDArgs * rda;
    IPTR          * args[TOTAL_ARGS] = { NULL, NULL, NULL, NULL, NULL };
    int             Return_Value;

    Return_Value = RETURN_OK;

    rda = ReadArgs(ARG_TEMPLATE, (LONG *)args, NULL);
    if (rda)
    {
        /* Set to a default.
         */
        if (args[ARG_FULL] == NOT_SET && args[ARG_TCB] == NOT_SET)
        {
            args[ARG_ALL] = (LONG *)-1;
        }

        Return_Value = Do_Status((LONG *)args[ARG_PROCESS],
                                 (LONG)args[ARG_FULL],
                                 (LONG)args[ARG_TCB],
                                 (LONG)args[ARG_ALL],
                                 (STRPTR *)&args[ARG_COM]
        );
    }
    else
    {
        PrintFault(IoErr(), "Fault");

        Return_Value = RETURN_ERROR;
    }

    FreeArgs(rda);

    return (Return_Value);

} /* main */



#define BUFFER_SIZE   100

void BSTRtoCSTR(BSTR, STRPTR, LONG);
void DumpInfo(LONG, LONG, LONG, LONG, STRPTR, LONG, LONG, LONG);

int Do_Status(LONG *Process, LONG Full, LONG Tcb, LONG All, STRPTR *Command)
{
	struct Process              *Proc;
	struct CommandLineInterface *CurrentCLI;
	char                         Buffer[BUFFER_SIZE]="fixme!";
	BOOL                         Done;
	LONG                        *Tasks;
	LONG                         CurrentTask;
	LONG                         NumberOfTasks;
	STRPTR                       msg;
	LONG                         ProcNum;
	LONG                         ProcStackSize;
	LONG                        *ProcGlobalVectorSize;

    /* Initialise some variables.
     */
    Proc                 = NULL;
    ProcNum              = 0;
    ProcStackSize        = 0;
    ProcGlobalVectorSize = 0;
    Tasks                = (LONG *)BADDR(DOSBase->dl_Root->rn_TaskArray);
    NumberOfTasks        = *Tasks++;
    CurrentTask          = 1;
    Done                 = FALSE;

    while (Done == FALSE)
    {
        msg = (STRPTR)*Tasks++;
        if (msg)
        {
            Proc                 = (struct Process *)(msg - sizeof(struct Task));
            ProcNum              = Proc->pr_TaskNum;
            ProcGlobalVectorSize = Proc->pr_GlobVec;
#warning Some structures are not correct, yet!
//            CurrentCLI           = (struct CommandLineInterface *)BADDR(Proc->pr_CLI);
//            ProcStackSize        = CurrentCLI->cli_DefaultStack * 4;

//            BSTRtoCSTR(CurrentCLI->cli_CommandName, &Buffer[0], BUFFER_SIZE);


            /* Work out what to display.
             */
            if (NULL != *Command)
            {
                if (strcmp((STRPTR)*Command, (STRPTR)&Buffer[0]) == 0)
                {
                    printf(" %ld\n", ProcNum);
                }
            }
            else if(Process != NULL)
            {
                if (CurrentTask == *Process)
                {
                    DumpInfo(ProcNum,
                             1234, //*ProcGlobalVectorSize,
                             ProcStackSize,
                             Proc->pr_Task.tc_Node.ln_Pri,
                             Buffer,
                             Full,
                             Tcb,
                             All
                    );
                }    
            }
            else
            {
                DumpInfo(ProcNum,
                         1234, //*ProcGlobalVectorSize,
                         ProcStackSize,
                         Proc->pr_Task.tc_Node.ln_Pri,
                         Buffer,
                         Full,
                         Tcb,
                         All
                );
            }
        }

        if (CurrentTask == NumberOfTasks || 
            (NULL != Process && CurrentTask == *Process))
        {
            Done = TRUE;
        }
        else
        {
            CurrentTask++;
        }
    }

    return 0;
} /* Do_Status */


void BSTRtoCSTR(BSTR In, STRPTR Out, LONG Length)
{
    LONG   TextLength;
    LONG   Counter;
    STRPTR Text;

    Text       = (STRPTR)BADDR(In);
    TextLength = Text[0];

    for (Counter = 1; Counter < TextLength + 1; Counter++)
    {
        Out[Counter - 1] = Text[Counter];
        
        /* Reached the buffer's maximum size?
         */
        if (Counter == Length)
        {
            break;
        }
    }

    Out[TextLength] = '\0';

} /* BSTRtoCSTR */


void DumpInfo(LONG Num,
              LONG GVec,
              LONG Stack,
              LONG Pri,
              STRPTR Name,
              LONG Full,
              LONG Tcb,
              LONG All
)
{
    if (Full != NOT_SET)
    {
        printf("Process %2ld: Stk %6ld, gv %3ld, Pri %3ld Loaded as command: %s\n",
               Num,
               Stack,
               GVec,
               Pri,
               (char *)&Name[0]
        );
    }
    else if (Tcb != NOT_SET)
    {
        printf("Process %2ld: Stk %6ld, gv %3ld, Pri %3ld\n",
               Num,
               Stack,
               GVec,
               Pri
        );
    }
    else if (All != NOT_SET)
    {
        printf("Process %2ld: Loaded as command: %s\n",
               Num,
               (char *)&Name[0]
        );
    }

} /* DumpInfo */
