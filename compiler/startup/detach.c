/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Common startup code
    Lang: english

    Use: To make a program detach itself from the launching CLI, use
         detach.o as your program's start file, before any other usual start
         file.

	 If you want the detached process to have a name different than the
	 detacher's one, define the following variable in your program's
	 source code:
	 
             STRPTR __detached_name = "My Preferred Program Name";
	     
	You can also decide to control exactly when to detach your program
	from the shell. To do so, simply use the function
	
	    Detach();
	    
	declared in
	
	    <aros/detach.h>
	    
	when you want to detach the program. Not using this function will
	result in your program being detached from the shell before the main()
	function is reached.
*/
#define DEBUG 0

#include <aros/config.h>
#include <dos/dos.h>
#include <dos/stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/startup.h>

int             __detached_manages_detach;
int             __detacher_go_away;
STRPTR          __detached_name;
LONG            __detached_return_value;
struct Process *__detacher_process;

AROS_UFP3S(LONG, __detach_trampoline,
AROS_UFPA(char *,argstr,A0),
AROS_UFPA(ULONG,argsize,D0),
AROS_UFPA(struct ExecBase *,SysBase,A6));

static void __startup_detach(void)
{
    struct CommandLineInterface *cli;
    struct Process              *newproc;
    BPTR                         mysegment = BNULL;
    STRPTR                       detached_name;

    D(bug("Entering __startup_detach()\n"));

    cli = Cli();
    /* Without a CLI detaching makes no sense, just jump to
       the real program.  */
    if (!cli)
    {
        D(bug("Wasn't started from cli.\n"));
        __startup_entries_next();
    }  
    else
    {
        D(bug("Was started from cli.\n"));
        mysegment = cli->cli_Module;
        cli->cli_Module = BNULL;
        BPTR in, out;

        detached_name = __detached_name ? __detached_name : (STRPTR) FindTask(NULL)->tc_Node.ln_Name;

        in  = OpenFromLock(DupLockFromFH(Input()));
        out = OpenFromLock(DupLockFromFH(Output()));
        if (IsInteractive(in))
            SetVBuf(in, NULL, BUF_LINE, -1);
        if (IsInteractive(out))
            SetVBuf(out, NULL, BUF_LINE, -1);
    
        {
            struct TagItem tags[] =
            {
                { NP_Seglist,   (IPTR)mysegment            },
                { NP_Entry,     (IPTR)&__detach_trampoline },
                { NP_Name,      (IPTR)detached_name        },
                { NP_Arguments, (IPTR)__argstr             },
                { NP_Input,     (IPTR)in                   },
                { NP_Output,    (IPTR)out                  },
                { NP_Cli,       TRUE                       },
                { TAG_DONE,     0                          }
            };

            __detacher_process = (struct Process *)FindTask(NULL);

            /* CreateNewProc() will take care of freeing the seglist */
            newproc = CreateNewProc(tags);
        }

        D(bug("Process \"%s\" = 0x%x.\n", detached_name, newproc));
        if (!newproc)
        {
            cli->cli_Module = mysegment;
            __detached_return_value = RETURN_ERROR;
        }
        else
            while (!__detacher_go_away) Wait(SIGF_SINGLE);

        D(bug("__detached_return_value = %d.\n", __detached_return_value));
        if (__detached_return_value != RETURN_OK)
        {
            PutStr(FindTask(NULL)->tc_Node.ln_Name); PutStr(": Failed to detach.\n");
        }
    
        if (newproc)
        {
            Forbid();
            Signal(&newproc->pr_Task, SIGF_SINGLE);
        }

        __startup_error = __detached_return_value;
    }

    D(bug("Leaving __startup_detach\n"));
}

ADD2SET(__startup_detach, PROGRAM_ENTRIES, -100);

void __Detach(LONG retval);

AROS_ENTRY(LONG, __detach_trampoline,
    AROS_UFHA(char *,argstr,A0),
    AROS_UFHA(ULONG,argsize,D0),
    struct ExecBase *,SysBase)
{
    AROS_USERFUNC_INIT
    
    LONG retval = RETURN_OK;
      
    D(bug("Entering __detach_trampoline()\n"));

    /* We have a CLI that is not 'attached' to a Shell,
     * and to get WB 1.3's C:Status to list us properly,
     * we need to set cli_Module to something.
     */
    Cli()->cli_Module =  (BPTR)-1;

    /* The program has two options: either take care of telling the detacher
       process when exactly to go away, via the Detach() function, or let this
       startup code take care of it. If __detached_manages_detach is TRUE, then
       the detached process handles it, otherwise we handle it.  */
    
    if (!__detached_manages_detach)
       __Detach(RETURN_OK);

    __startup_entries_next();
    
    /* At this point the detacher process might still be around, 
      If the program forgot to detach, or if it couldn't, but in any
      case we need to tell the detacher to go away.  */
    __Detach(retval);
    
    D(bug("Leaving __detach_trampoline\n"));

    return 0;
    
    AROS_USERFUNC_EXIT
}

void __Detach(LONG retval)
{
    D(bug("Entering __Detach(%d)\n", retval));

    if (__detacher_process != NULL)
    {
        __detached_return_value = retval;
        __detacher_go_away      = TRUE;

        SetSignal(0, SIGF_SINGLE);
        /* Tell the detacher process it can now go away */
        Signal(&__detacher_process->pr_Task, SIGF_SINGLE);

        /* Wait for it to say "goodbye" */
        Wait(SIGF_SINGLE);
        __detacher_process = NULL;

        Close(Input());
        SelectInput(BNULL);
        Close(Output());
        SelectOutput(BNULL);
    }

    D(bug("Leaving __Detach\n"));
}
