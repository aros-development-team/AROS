#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <clib/dos_protos.h>
#include <utility/tagitem.h>

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
    STRPTR args[1]={ 0 };
    struct RDArgs *rda;
    BPTR shell;
    STRPTR s1, s2, s3, buf;
    LONG error=0;
    
    rda=ReadArgs("FILE/A",(ULONG *)args,NULL);
    if(rda!=NULL)
    {
	s1=s2=(STRPTR)args[0];
	while(*s2++)
	    ;
	buf=(STRPTR)AllocVec(9+2*(s2-s1),MEMF_ANY);
	if(buf!=NULL)
	{
	    CopyMem("COMMAND ",buf,8);
	    s3=buf+8;
	    s2=s1;
	    *s3++='\"';
	    while(*s1)
	    {
	        if(*s1=='*'||*s1=='\"'||*s1=='\n')
	            *s3++='*';
	        if(*s1=='\n')
	            *s3++='n';
	        else
	            *s3++=*s1;
	        s1++;
	    }
	    *s3++='\"';
	    *s3=0;
	    shell=LoadSeg("c:shell");
	    if(shell)
	    {
	        RunCommand(shell,4096,buf,s3-buf);
	        UnLoadSeg(shell);
	    }
	    FreeVec(buf);
	}
	FreeArgs(rda);
    }else
	error=RETURN_FAIL;
    if(error)
	PrintFault(IoErr(),"Run");
    return error;
}