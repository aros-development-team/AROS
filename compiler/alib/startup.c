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

/* Don't define symbols before the entry point. */
extern struct ExecBase  *SysBase;
extern struct WBStartup *WBenchMsg;
extern int main (int argc, char ** argv);

extern jmp_buf __startup_jmp_buf;
extern LONG    __startup_error;

DECLARESET(INIT);
DECLARESET(EXIT);
DECLARESET(CTORS);
DECLARESET(DTORS);

/*
    This won't work for normal AmigaOS because you can't expect SysBase to be
    in A6. The correct way is to use *(struct ExecBase **)4 and because gcc
    emits strings for a certain function _before_ the code the program will
    crash immediately because the first element in the code won't be valid
    assembler code.

    970314 ldp: It will now work because of the asm-stub above.

*/

extern char *__argstr;
extern ULONG __argsize;
extern char **__argv;
extern int  __argc;
extern struct DosLibrary *DOSBase;

#warning TODO: reset and initialize the FPU
#warning TODO: resident startup
AROS_UFH3(LONG, __startup_entry,
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

    /* Do we have a CLI structure? */
    if (!myproc->pr_CLI)
    {
	/* Workbench startup. Get WBenchMsg and pass it to main() */

	WaitPort(&myproc->pr_MsgPort);
	WBenchMsg = (struct WBStartup *)GetMsg(&myproc->pr_MsgPort);
	__argv = (char **) WBenchMsg;
        __argc = 0;
    }

    if (!setjmp(__startup_jmp_buf))
    {
        if
        (
            !(__startup_error = set_open_libraries()) &&
            !(__startup_error = set_call_funcs(SETNAME(INIT), 1))
        )
        {
            /* ctors get called in inverse order than init funcs */
            set_call_funcs(SETNAME(CTORS), -1);

	    __startup_error = main (__argc, __argv);
        }

    }

    /* dtors get called in inverse order than exit funcs */
    {
        int n = 1;

        while (SETNAME(DTORS)[n]) ((VOID_FUNC)(SETNAME(DTORS)[n++]))();
    }

    set_call_funcs(SETNAME(EXIT), -1);
    set_close_libraries();

    /* Reply startup message to Workbench.
     * We Forbid() to avoid being UnLoadSeg()ed before we're really finished.
     */
    if (WBenchMsg)
    {
	Forbid();
	ReplyMsg((struct Message *)WBenchMsg);
    }

    CloseLibrary((struct Library *)DOSBase);

    return __startup_error;

    AROS_USERFUNC_EXIT
} /* entry */

/* if the programmer hasn't defined a symbol with the name __nocommandline
   then the code to handle the commandline will be included from the autoinit.lib
*/
//extern void __nocommandline(void);
extern int __nocommandline;
static void *__importcommandline __attribute__((unused)) = &__nocommandline;

/* pass these values to the command line handling function */
char *__argstr;
ULONG __argsize;

/* the command line handling functions will pass these values back to us */
char **__argv;
int  __argc;

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct WBStartup *WBenchMsg;
jmp_buf __startup_jmp_buf;
LONG __startup_error;

DEFINESET(CTORS);
DEFINESET(DTORS);
DEFINESET(INIT);
DEFINESET(EXIT);


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
