/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Common startup code
    Lang: english

    Use: To make a program detach itself from the launching CLI, use
         detach.o as your program's start file, before any other usual start
         file.

	 In your program's source code you have the option to use some
	 variables to determine the behaviour of the detaching procedure.

	 These variables are defined as "weak", so that defining them both
	 in this module and in your program doesn't produce linking errors.
	 Defining these variables in your program's source code makes it
	 possible to make your program stay independent from this module,
	 so that you can decide whether your program should be detacheable
	 or not just by adding a flag to the linker's command line and make
	 the program still work in case you decided to not make it
	 detacheable.

         These are the variables:

             1) LONG __detacher_must_wait_for_signal;

		Define the above variable only if you need the detacher
		process to wait for a signal to arrive, before exiting and
		effectively detaching your program.

		It must contain a bitmask suited for the use of
		exec.library/Wait()

	     2) struct Process *__detacher_process;

		The above variable will hold a pointer to the detacher
		process. Use it for anything you wish, but specially for
		sending to it the "termination" signal.

		This pointer is guaranteed to hold NULL when there's no
		detacher process around anymore.

                Remember to initialize it to NULL!

             3) STRPTR __detached_name;

		Define the above variable only if you want the detached
		process to have a name different than the detacher's one.
		Set its value to a pointer to a string holding the name you
		want the detached process to have.
		
	     4) LONG __detached_return_value;
	     
	        Define the above variable if you want the detacher
		process to exit with the value you set that variable 
                to. Useful when the detached process failed to
		initialize before detaching and wants this fact to
		be reported to the user.
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

extern LONG            __detacher_must_wait_for_signal __attribute__((weak));
extern struct Process *__detacher_process              __attribute__((weak));
extern STRPTR          __detached_name                 __attribute__((weak));
extern LONG            __detached_return_value         __attribute__((weak));

DECLARESET(PROGRAM_ENTRIES);

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

    DOSBase = (struct DosLibrary *)OpenLibrary(DOSNAME, 39);
    if (!DOSBase) return RETURN_FAIL;

    cli = Cli();
    /*
        We cannot be started from WorkBench
    */
    if (!cli) return RETURN_FAIL;

    mysegment = cli->cli_Module;
    cli->cli_Module = NULL;

    if (!__detached_name)
      __detached_name = FindTask(NULL)->tc_Node.ln_Name;

    {
        struct TagItem tags[] =
        {
	    { NP_Seglist,   (IPTR)mysegment                                   },
	    { NP_Entry,     (IPTR)SETELEM(__detach_entry, program_entries)[1] },
	    { NP_Name,      (IPTR)__detached_name                             },
	    { NP_Arguments, (IPTR)argstr                                      },
	    { NP_Cli,       TRUE                                              },
            { TAG_DONE,     0                                                 }
        };

	__detacher_process = (struct Process *)FindTask(NULL);

	/* CreateNewProc() will take care of freeing the seglist */
	newproc = CreateNewProc(tags);
    }

    CloseLibrary((struct Library *)DOSBase);

    if (newproc && __detacher_must_wait_for_signal)
        Wait(__detacher_must_wait_for_signal);

    __detacher_process = NULL;

    if (!newproc || __detached_return_value)
    {
        PutStr(__detached_name); PutStr(": Failed to detach.\n");
    }
    
    return newproc ? __detached_return_value : RETURN_FAIL;
    
    AROS_USERFUNC_EXIT
}

LONG            __detacher_must_wait_for_signal __attribute__((weak)) = 0;
struct Process *__detacher_process              __attribute__((weak)) = NULL;
STRPTR          __detached_name                 __attribute__((weak)) = NULL;
LONG            __detached_return_value         __attribute__((weak)) = RETURN_OK;

DEFINESET(PROGRAM_ENTRIES);
ADD2SET(__detach_entry, program_entries, 0);
