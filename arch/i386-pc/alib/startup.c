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

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
asm("
	.text

	move.l	4.w,a6
	jra	_entry(pc)
");
#endif

/* Don't define symbols before the entry point. */
extern struct ExecBase * SysBase;
extern struct WBStartup *WBenchMsg;
extern int main (int argc, char ** argv);

extern jmp_buf __startup_jmp_buf;
extern LONG __startup_error;

DECLARESET(INIT);
DECLARESET(EXIT);
DECLARESET(LIBS);

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

#warning TODO: reset and initialize the FPU
#warning TODO: resident startup
AROS_UFH3(LONG, entry,
    AROS_UFHA(char *,argstr,A0),
    AROS_UFHA(ULONG,argsize,D0),
    AROS_UFHA(struct ExecBase *,sysbase,A6)
)
{
    struct Process *myproc;

    __argstr  = argstr;
    __argsize = argsize;

    SysBase = *(struct ExecBase**)4UL;

    asm("finit");

    myproc = (struct Process *)FindTask(NULL);

    /* Do we have a CLI structure? */
    if (!myproc->pr_CLI)
    {
	/* Workbench startup. Get WBenchMsg and pass it to main() */

	WaitPort(&myproc->pr_MsgPort);
	WBenchMsg = (struct WBStartup *)GetMsg(&myproc->pr_MsgPort);
	__argv = (char **)WBenchMsg;
    }

    if
    (
        !(__startup_error = set_open_libraries(SETNAME(LIBS))) &&
        !(__startup_error = set_call_funcs(SETNAME(INIT), 1))
    )
    {
        /* Invoke main() */
        if (!setjmp(__startup_jmp_buf))
	    __startup_error = main (__argc, __argv);

    }

    set_call_funcs(SETNAME(EXIT), -1);
    set_close_libraries(SETNAME(LIBS));

    /* Reply startup message to Workbench.
     * We Forbid() to avoid being UnLoadSeg()ed before we're really finished.
     */
    if (WBenchMsg)
    {
	Forbid();
	ReplyMsg((struct Message *)WBenchMsg);
    }

    return __startup_error;
} /* entry */

/* if the programmer hasn't defined a symbol with the name __nocommandline
   then the code to handle the commandline will be included from the autoinit.lib
*/
extern void __nocommandline(void);
static void (*__importcommandline) = &__nocommandline;

/* pass these values to the command line handling function */
char *__argstr;
ULONG __argsize;

/* the command line handling functions will pass these values back to us */
char **__argv;
int  __argc;

struct ExecBase *SysBase;
struct WBStartup *WBenchMsg;
jmp_buf __startup_jmp_buf;
LONG __startup_error;

DEFINESET(INIT);
DEFINESET(EXIT);
DEFINESET(LIBS);

/*	Stub function for GCC __main().

	The __main() function is originally used for C++ style constructors
	and destructors in C. This replacement does nothing and gets rid of
	linker-errors about references to __main().
*/

void __main(void)
{
/* Do nothing. */
}
