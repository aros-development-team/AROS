/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/27 00:22:36  ldp
    Include proto instead of clib

    Revision 1.6  1996/10/10 13:12:11  digulla
    Wrong parameter for Lock()

    Revision 1.5  1996/09/17 16:42:59  digulla
    Use general startup code

    Revision 1.4  1996/09/13 17:52:09  digulla
    Use IPTR

    Revision 1.3  1996/08/13 15:34:04  digulla
    #include <exec/execbase.h> was missing

    Revision 1.2  1996/08/01 17:40:43  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

int main (int argc, char ** argv)
{
    STRPTR args[1]={ 0 };
    struct RDArgs *rda;
    BPTR dir;
    STRPTR buf;
    ULONG i;
    struct FileInfoBlock *fib;
    LONG error=0;

    rda=ReadArgs("DIR",(IPTR *)args,NULL);
    if(rda!=NULL)
    {
	if(args[0])
	{
	    dir=Lock(args[0],SHARED_LOCK);
	    if(dir)
	    {
		fib=AllocDosObject(DOS_FIB,NULL);
		if(fib!=NULL)
		{
		    if(Examine(dir,fib))
		    {
			if(fib->fib_DirEntryType>0)
			    dir=CurrentDir(dir);
			else
			{
			    SetIoErr(ERROR_DIR_NOT_FOUND);
			    error=RETURN_ERROR;
			}
		    }else
			error=RETURN_ERROR;
		    FreeDosObject(DOS_FIB,fib);
		}else
		{
		    SetIoErr(ERROR_NO_FREE_STORE);
		    error=RETURN_ERROR;
		}
		UnLock(dir);
	    }else
		error=RETURN_ERROR;
	}else
	{
	    dir=CurrentDir(0);
	    for(i=256;;i+=256)
	    {
		buf=AllocVec(i,MEMF_ANY);
		if(buf==NULL)
		{
		    SetIoErr(ERROR_NO_FREE_STORE);
		    error=RETURN_ERROR;
		    break;
		}
		if(NameFromLock(dir,buf,i))
		{
		    if(FPuts(Output(),buf)<0||
		       FPuts(Output(),"\n")<0)
			error=RETURN_ERROR;
		    FreeVec(buf);
		    break;
		}
		FreeVec(buf);
		if(IoErr()!=ERROR_LINE_TOO_LONG)
		{
		    error=RETURN_ERROR;
		    break;
		}
	    }
	    CurrentDir(dir);
	}
	FreeArgs(rda);
    }else
	error=RETURN_FAIL;
    if(error)
	PrintFault(IoErr(),"CD");
    return error;
}
