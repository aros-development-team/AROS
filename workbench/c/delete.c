/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Delete CLI command
    Lang: english
*/

#include <exec/memory.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

static const char version[] = "$VER: delete 41.1 (14.3.1997)\n";

int main (int argc, char ** argv)
{
    STRPTR args[1]={ 0 };
    struct RDArgs *rda;
    LONG error=0;

    rda=ReadArgs("FILE/A",(IPTR *)args,NULL);

    if(rda!=NULL)
    {
	if (!DeleteFile(args[0]))
	    error = RETURN_ERROR;

	FreeArgs(rda);
    }
    else
	error=RETURN_FAIL;

    if(error)
	PrintFault(IoErr(),"Delete");
    return error;
}
