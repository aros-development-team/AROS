
/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Common startup code
    Lang: english
*/
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

/* Don't define symbols before the entry point. */
extern struct ExecBase  *SysBase;
extern struct WBStartup *WBenchMsg;
extern int main(int argc, char ** argv);
int (*__main_function_ptr)(int argc, char ** argv) __attribute__((__weak__)) = main;

DECLARESET(INIT);
DECLARESET(EXIT);
DECLARESET(CTORS);
DECLARESET(DTORS);
DECLARESET(PROGRAM_ENTRIES);

/*
    This won't work for normal AmigaOS because you can't expect SysBase to be
    in A6. The correct way is to use *(struct ExecBase **)4 and because GCC
    emits strings for a certain function _before_ the code the program will
    crash immediately because the first element in the code won't be valid
    assembler code.
*/

extern char *__argstr;
extern ULONG __argsize;
extern char **__argv;
extern int  __argc;
extern struct DosLibrary *DOSBase;

extern struct aros_startup __aros_startup;

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

    SysBase = sysbase;

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

    CloseLibrary((struct Library *)DOSBase);

    return __aros_startup.as_startup_error;

    AROS_USERFUNC_EXIT
} /* entry */

/* if the programmer hasn't defined a symbol with the name __nocommandline
   then the code to handle the commandline will be included from the autoinit.lib
*/
extern int __nocommandline;
asm(".set __importcommandline, __nocommandline");

/* pass these values to the command line handling function */
char *__argstr;
ULONG __argsize;

/* the command line handling functions will pass these values back to us */
char **__argv;
int  __argc;

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct WBStartup *WBenchMsg;
static struct aros_startup __aros_startup;

DEFINESET(CTORS);
DEFINESET(DTORS);
DEFINESET(INIT);
DEFINESET(EXIT);
DEFINESET(PROGRAM_ENTRIES);
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
