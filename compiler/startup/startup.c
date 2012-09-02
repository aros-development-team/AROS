
/*
    Copyright � 1995-2009, The AROS Development Team. All rights reserved.
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

struct DosLibrary *DOSBase;
extern const LONG const __aros_libreq_DOSBase __attribute__((weak));

THIS_PROGRAM_HANDLES_SYMBOLSET(PROGRAM_ENTRIES)

extern int main(int argc, char ** argv);
int (*__main_function_ptr)(int argc, char ** argv) __attribute__((__weak__)) = main;

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

extern void __startup_entries_init(void);

/* Guarantee that __startup_entry is placed at the beginning of the binary */
__startup AROS_PROCH(__startup_entry, argstr, argsize, SysBase)
{
    AROS_PROCFUNC_INIT

    D(bug("Entering __startup_entry(\"%s\", %d, %x)\n", argstr, argsize, SysBase));

    /*
        No one program will be able to do anything useful without the dos.library,
        so we open it here instead of using the automatic opening system
    */
    DOSBase = (struct DosLibrary *)OpenLibrary(DOSNAME, 0);
    if (!DOSBase) return RETURN_FAIL;
    if (((struct Library *)DOSBase)->lib_Version < __aros_libreq_DOSBase)
        return RETURN_FAIL;

    __argstr  = argstr;
    __argsize = argsize;
    __startup_error = RETURN_FAIL;

    __startup_entries_init();
    __startup_entries_next();

    CloseLibrary((struct Library *)DOSBase);

    D(bug("Leaving __startup_entry\n"));

    return __startup_error;

    AROS_PROCFUNC_EXIT
} /* entry */


static void __startup_main(struct ExecBase *SysBase)
{
    D(bug("Entering __startup_main\n"));

    /* Invoke the main function. A weak symbol is used as function name so that
       it can be overridden (for *nix stuff, for instance).  */
    __startup_error = (*__main_function_ptr) (__argc, __argv);

    D(bug("Leaving __startup_main\n"));
}

ADD2SET(__startup_main, PROGRAM_ENTRIES, 127);


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
