/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang:
*/
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <string.h>
#include "shcommands.h"


AROS_SH5(Echo, 41.1,
AROS_SHA(STRPTR *, , STRING, /M,   NULL),
AROS_SHA(BOOL,     , NOLINE, /S,   FALSE),
AROS_SHA(ULONG *,  , FIRST,  /K/N, NULL),
AROS_SHA(ULONG *,  , LEN,    /K/N, NULL),
AROS_SHA(STRPTR,   , TO,     /K,   NULL))
{
    AROS_SHCOMMAND_INIT

    STRPTR *a, b;
    ULONG l, max=~0ul;
    BPTR out=Output();
    LONG error=0;

    #define ERROR(a) { error=a; goto end; }

    if(SHArg(LEN))
	max=*SHArg(LEN);

    if(SHArg(TO))
    {
	out=Open(SHArg(TO),MODE_NEWFILE);
	if(!out)
	    ERROR(RETURN_ERROR);
    }

    a=SHArg(STRING);
    while(*a!=NULL)
    {
	b=*a;
	while(*b++);

	l=b-*a-1;
	b=*a;
	if(SHArg(FIRST)&&*SHArg(FIRST))
	{
	    if(*SHArg(FIRST)-1<l)
		b+=*SHArg(FIRST)-1;
	    else
		b+=l;
	}else
	    if(l>max)
		b+=l-max;
	l=max;
	while(l--&&*b)
	    if(FPutC(out,*b++)<0)
		ERROR(RETURN_ERROR);
	a++;
	if(*a)
	    if(FPutC(out,' ')<0)
		ERROR(RETURN_ERROR);
    }
    if(!SHArg(NOLINE))
	if(FPutC(out,'\n')<0)
	    ERROR(RETURN_ERROR);
    if(!Flush(out))
	ERROR(RETURN_ERROR);
end:
    if(SHArg(TO) && out)
	Close(out);

    return error;

    AROS_SHCOMMAND_EXIT
}





