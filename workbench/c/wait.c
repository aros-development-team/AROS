/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/09/13 17:52:12  digulla
    Use IPTR

    Revision 1.3  1996/08/13 15:34:04  digulla
    #include <exec/execbase.h> was missing

    Revision 1.2  1996/08/01 17:40:46  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>

CALLENTRY /* Before the first symbol */

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;

static LONG tinymain(void);

LONG entry(struct ExecBase *sysbase)
{
    LONG error=RETURN_FAIL;
    SysBase=sysbase;
    DOSBase=(struct DosLibrary *)OpenLibrary("dos.library",39);
    if(DOSBase!=NULL)
    {
	error=tinymain();
	CloseLibrary((struct Library *)DOSBase);
    }
    return error;
}

static LONG tinymain(void)
{
    IPTR args[4]={ 0, 0, 0, 0 };
    struct RDArgs *rda;
    LONG error=0;
    ULONG delay = 1;
#define ERROR(a) { error=a; goto end; }

    rda=ReadArgs("time/N,SEC=SECS/S,MIN=MINS/S,UNTIL/K",args,NULL);
    if(rda==NULL)
	ERROR(RETURN_FAIL);

    if (args[0])
	delay = *((ULONG *)args[0]);

    if (args[2])
	delay *= 60L;

    if (delay > 0)
	Delay (delay * 50L);

end:
    FreeArgs(rda);
    if(error)
	PrintFault(IoErr(),"Echo");
    return error;
}
