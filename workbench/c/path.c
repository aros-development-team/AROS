#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
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
    struct RDArgs *rda;
    ULONG args[6]={ 0, 0, 0, 0, 0, 0 };

    rda=ReadArgs("PATH/M,ADD/S,SHOW/S,RESET/S,REMOVE/S,QUIET/S",args,NULL);
    if(rda!=NULL)
    {
	STRPTR *names=(STRPTR *)args[0];
	BPTR *cur, *next;
	struct CommandLineInterface *cli;
	cli=Cli();
	cur=(BPTR *)BADDR(cli->cli_CommandDir);
	while(cur)
	{
	    next=(BPTR *)cur[0];
	    UnLock(cur[1]);
	    FreeVec(cur);
	    cur=next;
	}
	cur=&cli->cli_CommandDir;
	while(*names!=NULL)
	{
	    next=(BPTR *)AllocVec(2*sizeof(BPTR),MEMF_ANY);
	    next[1]=Lock(*names,SHARED_LOCK);
	    if(!next[1])
	    {
	        FreeVec(next);
	        break;
	    }
	    cur[0]=MKBADDR(next);
	    cur=next;
	    if(!args[5])
	        VPrintf("%s added.\n",(ULONG *)names);
	    names++;
	}
	cur[0]=0;
	FreeArgs(rda);
    }
    return 0;
}