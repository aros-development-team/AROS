#include <exec/memory.h>
#include <exec/execbase.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
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
    STRPTR args[2]={ NULL, NULL };
    struct RDArgs *rda;
    BPTR dir;
    LONG error=0;
    
    rda=ReadArgs("DEVICE/A,DIR/A",(ULONG *)args,NULL);
    if(rda!=NULL)
    {
        dir=Lock(args[1],SHARED_LOCK);
        if(dir)
        {
            STRPTR s=args[0];
            while(*s)
                if(*s++==':')
                    s[-1]=0;
            AssignLock(args[0],dir);
        }
	FreeArgs(rda);
    }else
	error=RETURN_FAIL;
    if(error)
	PrintFault(IoErr(),"MakeDir");
    return error;
}