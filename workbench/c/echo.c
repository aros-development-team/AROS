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
    ULONG args[5]={ 0, 0, 0, 0, 0 };
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
    FreeArgs(rda);
    if(error)
        PrintFault(IoErr(),"Echo");
    return error;
}
