#include <exec/libraries.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;

__AROS_LH0(LONG,entry,struct ExecBase *,sysbase,,)
{
    __AROS_FUNC_INIT
    
    SysBase=sysbase;
    DOSBase=(struct DosLibrary *)OpenLibrary("dos.library",39);
    if(DOSBase!=NULL)
    {
        Write(Output(),"hello, world\n",13);
        CloseLibrary((struct Library *)DOSBase);
    }
    return 0;
    __AROS_FUNC_EXIT
}
