/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/24 15:50:20  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/10/24 01:41:05  aros
    Added __main()

    Revision 1.3  1996/10/19 17:02:33  aros
    Added support for malloc() and free(). I added it to the general startup,
    because it uses only a few bytes and with MemHeaders, it would have been
    quite a mess :-) This way, everything is *very* simple.

    Revision 1.2  1996/09/17 18:07:19  digulla
    DOSBase and SysBase are now declared in the respective header files.
    The type of DOSBase is "struct DosLibrary *". Fixed everywhere

    Revision 1.1  1996/09/17 16:42:45  digulla
    General startup module: Defines two global symbols: SysBase and DOSBase
	and opens dos.library


    Desc: Common startup code
    Lang: english
*/
#include <dos/dos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

/* Don't define symbols before the entry point. */
extern int main (int argc, char ** argv);
extern APTR __startup_mempool; /* malloc() and free() */

AROS_LH0(LONG,entry,struct ExecBase *,sysbase,,)
{
    AROS_LIBFUNC_INIT
    LONG error=RETURN_FAIL;

    SysBase=sysbase;
    DOSBase=(struct DosLibrary *)OpenLibrary(DOSNAME,39);
    if(DOSBase!=NULL)
    {
	char * argv[2];

	argv[0] = "dummy";
	argv[1] = NULL;

	error = main (1, argv);

	CloseLibrary((struct Library *)DOSBase);
    }

    if (__startup_mempool)
	DeletePool (__startup_mempool);

    return error;
    AROS_LIBFUNC_EXIT
}

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;

APTR __startup_mempool = NULL;

/*	Stub function for GCC __main().

	The __main() function is originally used for C++ style constructors
	and destructors in C. This replacement does nothing and gets rid of
	linker-errors about references to __main().
*/

void __main(void)
{
/* Do nothing. */
}

