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

static const char version[] = "$VER: echo 41.1 (14.3.1997)\n";

int __nocommandline = 1;

int main (void)
{
    IPTR args[5]={ 0, 0, 0, 0, 0 };
    struct RDArgs *rda;
    STRPTR *a, b;
    ULONG l, max=~0ul;
    BPTR out=Output();
    LONG error=0;
#define ERROR(a) { error=a; goto end; }

    rda=ReadArgs("/M,NOLINE/S,FIRST/K/N,LEN/K/N,TO/K",args,NULL);
    if(rda==NULL)
	ERROR(RETURN_FAIL);

    if(args[3])
	max=*(ULONG *)args[3];

    if(args[4])
    {
	out=Open((STRPTR)args[4],MODE_NEWFILE);
	if(!out)
	    ERROR(RETURN_ERROR);
    }

    a=(STRPTR *)args[0];
    while(*a!=NULL)
    {
	b=*a;
	while(*b++)
	    ;
	l=b-*a-1;
	b=*a;
	if(args[2]&&*(ULONG *)args[2])
	{
	    if(*(ULONG *)args[2]-1<l)
		b+=*(ULONG *)args[2]-1;
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
    if(!args[1])
	if(FPutC(out,'\n')<0)
	    ERROR(RETURN_ERROR);
    if(!Flush(out))
	ERROR(RETURN_ERROR);
end:
    if(args[4])
	Close(out);
    if (rda)
        FreeArgs(rda);
    if(error)
	PrintFault(IoErr(),"Echo");
    return error;
}





