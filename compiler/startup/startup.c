
/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Common startup code
    Lang: english
*/
#define DEBUG 0

#include <aros/config.h>
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

extern int main(int argc, char ** argv);
int (*__main_function_ptr)(int argc, char ** argv) __attribute__((__weak__)) = main;

DEFINESET(PROGRAM_ENTRIES);

/* if the programmer hasn't defined a symbol with the name __nocommandline
   then the code to handle the commandline will be included from the autoinit.lib
*/
extern int __nocommandline;
asm(".set __importcommandline, __nocommandline");

/* if the programmer hasn't defined a symbol with the name __nostdiowin
   then the code to open a window for stdio will be included from the autoinit.lib
*/
extern int __nostdiowin;
asm(".set __importstdiowin, __nostdiowin");

/* if the programmer hasn't defined a symbol with the name __nowbsupport
   then the code to handle support for programs started from WB will be included from
   the autoinit.lib
*/
extern int __nowbsupport;
asm(".set __importnowbsupport, __nowbsupport");

/* if the programmer hasn't defined a symbol with the name __noinitexitsets
   then the code to handle support for calling the INIT, EXIT symbolset functions
   and the autoopening of libraries is called from the autoinit.lib
*/
extern int __noinitexitsets;
asm(".set __importnoinitexitsets, __noinitexitsets");

static void __startup_entries_init(void);

struct aros_startup __aros_startup;

/* Guarantee that __startup_entry is placed at the beginning of the binary */
AROS_UFP3(LONG, __startup_entry,
    AROS_UFHA(char *,argstr,A0),
    AROS_UFHA(ULONG,argsize,D0),
    AROS_UFHA(struct ExecBase *,sysbase,A6)
) __attribute__((section(".aros.startup")));

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
    __aros_startup.as_startup_error = RETURN_FAIL;

    __startup_entries_init();
    __startup_entries_next();

    CloseLibrary((struct Library *)DOSBase);

    D(bug("Leaving __startup_entry\n"));

    return __aros_startup.as_startup_error;

    AROS_USERFUNC_EXIT
} /* entry */


static void __startup_setjmp(void)
{
    D(bug("Entering __startup_setjmp\n"));

    if (setjmp(__aros_startup.as_startup_jmp_buf) == 0)
    {
        __startup_entries_next();
    }

    D(bug("Leaving __startup_setjmp\n"));
}


static void __startup_main(void)
{
    D(bug("Entering __startup_main\n"));

    /* Invoke the main function. A weak symbol is used as function name so that
       it can be overridden (for *nix stuff, for instance).  */
    __aros_startup.as_startup_error = (*__main_function_ptr) (__argc, __argv);

    D(bug("Leaving __startup_main\n"));
}

ADD2SET(__startup_setjmp, program_entries, -10);
ADD2SET(__startup_main, program_entries, 127);


static int __startup_entry_pos;

void __startup_entries_init(void)
{
    __startup_entry_pos = 1;
}

void __startup_entries_next(void)
{
    void (*entry_func)(void);
 
    entry_func = SETNAME(PROGRAM_ENTRIES)[__startup_entry_pos];
    if (entry_func)
    {
        __startup_entry_pos++;
        entry_func();
    }
}


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
