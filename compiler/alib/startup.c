/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
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
extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;
extern int main (int argc, char ** argv);

__AROS_LH0(LONG,entry,struct ExecBase *,sysbase,,)
{
    __AROS_FUNC_INIT
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
    return error;
    __AROS_FUNC_EXIT
}

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
