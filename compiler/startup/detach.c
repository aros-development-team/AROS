/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

#include <aros/config.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE & defined __mc68000__)
asm
(
    ".text\n"
    "\n"
    "move.l      4.w,a6\n"
    "bra         _detach_entry\n"
);
#endif

int             __detached_manages_detach;
STRPTR          __detached_name;
LONG            __detached_return_value;
struct Process *__detacher_process;

DECLARESET(PROGRAM_ENTRIES);

AROS_UFP3(static LONG, __detach_trampoline,
AROS_UFHA(char *,argstr,A0),
AROS_UFHA(ULONG,argsize,D0),
AROS_UFHA(struct ExecBase *,SysBase,A6));

AROS_UFH3(static LONG, __detach_entry,
AROS_UFHA(char *,argstr,A0),
AROS_UFHA(ULONG,argsize,D0),
AROS_UFHA(struct ExecBase *,SysBase,A6))
{
    AROS_USERFUNC_INIT

    struct DosLibrary           *DOSBase;
    struct CommandLineInterface *cli;
    struct Process              *newproc;
    BPTR                         mysegment = NULL;
    STRPTR                       detached_name;

    DOSBase = (struct DosLibrary *)OpenLibrary(DOSNAME, 39);
    if (!DOSBase) return RETURN_FAIL;

    cli = Cli();
    /*
        We cannot be started from WorkBench
    */
    if (!cli) return RETURN_FAIL;

    mysegment = cli->cli_Module;
    cli->cli_Module = NULL;

    detached_name = __detached_name ? __detached_name : (STRPTR)FindTask(NULL)->tc_Node.ln_Name;
    
    {
        struct TagItem tags[] =
        {
	    { NP_Seglist,   (IPTR)mysegment            },
	    { NP_Entry,     (IPTR)&__detach_trampoline },
	    { NP_Name,      (IPTR)detached_name        },
	    { NP_Arguments, (IPTR)argstr               },
	    { NP_Cli,       TRUE                       },
            { TAG_DONE,     0                          }
        };

	__detacher_process = (struct Process *)FindTask(NULL);

	/* CreateNewProc() will take care of freeing the seglist */
	newproc = CreateNewProc(tags);
    }

    CloseLibrary((struct Library *)DOSBase);
    
    if (!newproc)
    {
        cli->cli_Module = mysegment;
	__detached_return_value = RETURN_ERROR;
    }
    else
        Wait(SIGF_SINGLE);

    if (__detached_return_value != RETURN_OK)
    {
        PutStr(FindTask(NULL)->tc_Node.ln_Name); PutStr(": Failed to detach.\n");
    }
    
    if (newproc)
    {
        Forbid();
	Signal(&newproc->pr_Task, SIGF_SINGLE);
    }
    
    return __detached_return_value;
    
    AROS_USERFUNC_EXIT
}

DEFINESET(PROGRAM_ENTRIES);
ADD2SET(__detach_entry, program_entries, 0);

void __Detach(LONG retval);

AROS_UFH3(static LONG, __detach_trampoline,
AROS_UFHA(char *,argstr,A0),
AROS_UFHA(ULONG,argsize,D0),
AROS_UFHA(struct ExecBase *,SysBase,A6))
{
    AROS_USERFUNC_INIT
    
    LONG retval;
      
    /* The program has two options: either take care of telling the detacher
       process when exactly to go away, via the Detach() function, or let this
       startup code take care of it. If __detached_manages_detach is TRUE, then
       the detached process handles it, otherwise we handle it.  */
    
    if (!__detached_manages_detach)
       __Detach(RETURN_OK);
    
    retval = AROS_UFC3(LONG, SETELEM(__detach_entry, program_entries)[1],
             AROS_UFHA(char *,argstr,A0),
             AROS_UFHA(ULONG,argsize,D0),
             AROS_UFHA(struct ExecBase *,SysBase,A6));
    
    /* At this point the detacher process might still be around, 
      If the program forgot to detach, or if it couldn't, but in any
      case we need to tell the detacher to go away.  */
    __Detach(retval);
    
    return 0;	  
    
    AROS_USERFUNC_EXIT
}

void __Detach(LONG retval)
{
    if (__detacher_process != NULL)
    {
        __detached_return_value = retval;
	/* Tell the detacher process it can now go away */
        Signal(&__detacher_process->pr_Task, SIGF_SINGLE);
	
	/* Wait for it to say "goodbye" */
	Wait(SIGF_SINGLE);
	__detacher_process = NULL;
    }
}
