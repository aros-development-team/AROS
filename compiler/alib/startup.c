/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Common startup code
    Lang: english
*/
#include <setjmp.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/asmcall.h>

/* Don't define symbols before the entry point. */
extern struct ExecBase * SysBase;
extern int main (int argc, char ** argv);
extern APTR __startup_mempool; /* malloc() and free() */
extern jmp_buf __startup_jmp_buf;
extern LONG __startup_error;

/*
    TODO: This won't work for normal AmigaOS for two reasons:
    1. You can't expect SysBase to be in A6. The correct way
       is to use *(struct ExecBase **)4.
    2. Amiga gcc puts strings into the code section - and since
       all gccs emit strings for a certain function _before_ the
       code the program will crash immediately.
*/
AROS_UFH1(LONG, entry,
    AROS_UFHA(struct ExecBase *,sysbase,A6)
)
{
    __startup_error = RETURN_FAIL;

    SysBase=sysbase;
    DOSBase=(struct DosLibrary *)OpenLibrary(DOSNAME,39);

    if(DOSBase!=NULL)
    {
	char * argv[2];

	argv[0] = "dummy";
	argv[1] = NULL;

	if (!setjmp (__startup_jmp_buf))
	    __startup_error = main (1, argv);

	CloseLibrary((struct Library *)DOSBase);
    }

    if (__startup_mempool)
	DeletePool (__startup_mempool);

    return __startup_error;
} /* entry */

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;

APTR __startup_mempool = NULL;
jmp_buf __startup_jmp_buf;
LONG __startup_error;

/*	Stub function for GCC __main().

	The __main() function is originally used for C++ style constructors
	and destructors in C. This replacement does nothing and gets rid of
	linker-errors about references to __main().
*/

void __main(void)
{
/* Do nothing. */
}

