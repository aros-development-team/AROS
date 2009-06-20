
/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Common startup code
    Lang: english
*/
#define DEBUG 0

#include <aros/config.h>
#include <setjmp.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <workbench/startup.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/startup.h>

#include "etask.h"

THIS_PROGRAM_HANDLES_SYMBOLSETS

/* pass these values to the command line handling function */
char *__argstr;
ULONG __argsize;

/* the command line handling functions will pass these values back to us */
char **__argv;
int  __argc;

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct WBStartup *WBenchMsg;

extern int main(int argc, char ** argv);
int (*__main_function_ptr)(int argc, char ** argv) __attribute__((__weak__)) = main;


DEFINESET(CTORS);
DEFINESET(DTORS);
DEFINESET(INIT);
DEFINESET(EXIT);
DEFINESET(PROGRAM_ENTRIES);

/* if the programmer hasn't defined a symbol with the name __nocommandline
   then the code to handle the commandline will be included from the autoinit.lib
*/
extern int __nocommandline;
asm(".set __importcommandline, __nocommandline");

/* programmers can define the __stdiowin for opening the win that will be used for
   IO to the standard file descriptors.
   If none is provided a default value will be used
*/
extern char __stdiowin[];

static struct aros_startup __aros_startup;

AROS_UFP3(static LONG, __startup_entry,
    AROS_UFHA(char *,argstr,A0),
    AROS_UFHA(ULONG,argsize,D0),
    AROS_UFHA(struct ExecBase *,sysbase,A6)
) __attribute__((section(".aros.startup")));

#warning TODO: reset and initialize the FPU
#warning TODO: resident startup
AROS_UFH3(static LONG, __startup_entry,
    AROS_UFHA(char *,argstr,A0),
    AROS_UFHA(ULONG,argsize,D0),
    AROS_UFHA(struct ExecBase *,sysbase,A6)
)
{
    AROS_USERFUNC_INIT

    struct Process *myproc;
    BPTR win = NULL;
    BPTR old_in, old_out, old_err;
    
    SysBase = sysbase;

    D(bug("Entering __startup_entry(\"%s\", %d, %x)\n", argstr, argsize, SysBase));

    /*
        No one program will be able to do anything useful without the dos.library,
        so we open it here instead of using the automatic opening system
    */
    DOSBase = (struct DosLibrary *)OpenLibrary(DOSNAME, 39);
    if (!DOSBase) return RETURN_FAIL;

    __argstr  = argstr;
    __argsize = argsize;

    myproc = (struct Process *)FindTask(NULL);

    GetIntETask(myproc)->iet_startup = &__aros_startup;

    /* Do we have a CLI structure? */
    if (!myproc->pr_CLI)
    {
	/* Workbench startup. Get WBenchMsg and pass it to main() */

	WaitPort(&myproc->pr_MsgPort);
	WBenchMsg = (struct WBStartup *)GetMsg(&myproc->pr_MsgPort);
	__argv = (char **) WBenchMsg;
        __argc = 0;

	D(bug("[startup] Started from Workbench\n"));
        if(__stdiowin[0]) {
            D(bug("[startup] Opening console window: %s\n", __stdiowin));
            win = Open(__stdiowin, MODE_OLDFILE);
            if (win) {
                D(bug("[startup] Success!\n"));
		old_in = SelectInput(win);
		old_out = SelectOutput(win);
		old_err = SelectError(win);
	    }
	}
    }

    __aros_startup.as_startup_error = RETURN_FAIL;
    
    if (set_open_libraries())
    {
        if
	(
	    setjmp(__aros_startup.as_startup_jmp_buf) == 0 &&
            set_call_funcs(SETNAME(INIT), 1, 1)
	)
	{
            /* ctors/dtors get called in inverse order than init funcs */
            set_call_funcs(SETNAME(CTORS), -1, 0);

	    /* Invoke the main function. A weak symbol is used as function name so that
	       it can be overridden (for *nix stuff, for instance).  */
            __aros_startup.as_startup_error = (*__main_function_ptr) (__argc, __argv);
		  
            set_call_funcs(SETNAME(DTORS), 1, 0);
        }
        set_call_funcs(SETNAME(EXIT), -1, 0);
    }
    set_close_libraries();
    
    
    /* Reply startup message to Workbench */
    if (WBenchMsg)
    {
        Forbid(); /* make sure we're not UnLoadseg()ed before we're really done */
        ReplyMsg((struct Message *) WBenchMsg);
    }

    if (win) {
        SelectInput(old_in);
        SelectOutput(old_out);
        SelectError(old_err);
        Close(win);
    }
    CloseLibrary((struct Library *)DOSBase);

    D(bug("Leaving __startup_entry\n"));

    return __aros_startup.as_startup_error;

    AROS_USERFUNC_EXIT
} /* entry */

ADD2SET(__startup_entry, program_entries, 0);


/*
    Stub function for GCC __main().

    The __main() function is originally used for C++ style constructors
    and destructors in C. This replacement does nothing and gets rid of
    linker-errors about references to __main().
*/
#ifdef AROS_NEEDS___MAIN
void __main(void)
{
    /* Do nothing. */
}
#endif
