#define DEBUG 0

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include "../kernel/hostinterface.h"
#include "hostlib_intern.h"

#include LC_LIBDEFS_FILE

AROS_LH2(void *, HostLib_Open,
         AROS_LHA(const char *, filename, A0),
         AROS_LHA(char **,      error,    A1),
         struct HostLibBase *, HostLibBase, 1, HostLib)
{
    AROS_LIBFUNC_INIT

    return HostLibBase->HostIFace->HostLib_Open(filename, error);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(BOOL, HostLib_Close,
         AROS_LHA(void *,  handle, A0),
         AROS_LHA(char **, error,  A1),
         struct HostLibBase *, HostLibBase, 2, HostLib)
{
    AROS_LIBFUNC_INIT

    return HostLibBase->HostIFace->HostLib_Close(handle, error);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void *, HostLib_GetPointer,
         AROS_LHA(void *,       handle, A0),
         AROS_LHA(const char *, symbol, A1),
         AROS_LHA(char **,      error,  A2),
         struct HostLibBase *, HostLibBase, 3, HostLib)
{
    AROS_LIBFUNC_INIT

    return HostLibBase->HostIFace->HostLib_GetPointer(handle, symbol, error);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, HostLib_FreeErrorStr,
    	 AROS_LHA(char *, error, A0),
    	 struct HostLibBase *, HostLibBase, 4, HostLib)
{
    AROS_LIBFUNC_INIT

    HostLibBase->HostIFace->HostLib_FreeErrorStr(error);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(APTR *, HostLib_GetInterface,
    	 AROS_LHA(void *,  handle,         A0),
    	 AROS_LHA(const char **, symtable, A1),
    	 AROS_LHA(ULONG *, unresolved,     A2),
    	 struct HostLibBase *, HostLibBase, 5, HostLib)
{
    AROS_LIBFUNC_INIT

    const char **c;
    ULONG cnt = 0;
    APTR *iface = NULL;

    for (c = symtable; *c; c++)
        cnt += sizeof(APTR);
    if (cnt) {
    	iface = AllocVec(cnt, MEMF_CLEAR);
    	if (iface) {
    	    cnt = HostLibBase->HostIFace->HostLib_GetInterface(handle, symtable, iface);
    	    if (unresolved)
    	        *unresolved = cnt;
    	}
    }
    return iface;
    
    AROS_LIBFUNC_EXIT
}

AROS_LH1I(void, HostLib_DropInterface,
   	 AROS_LHA(APTR *, interface, A0),
   	 struct HostLibBase *, HostLibBase, 6, HostLib)
{
    AROS_LIBFUNC_INIT

    FreeVec(interface);

    AROS_LIBFUNC_EXIT
}

/* auto init */
static int HostLib_Init(LIBBASETYPEPTR LIBBASE)
{
    struct UtilityBase *UlitityBase;
    APTR KernelBase;
    struct TagItem *BootInfo;
    
    UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 36);
    D(bug("[hostlib] UtilityBase = 0x%08lX\n", UtilityBase));
    if (UtilityBase) {
        KernelBase = OpenResource("kernel.resource");
        D(bug("[hostlib] KernelBase = 0x%08lX\n", KernelBase));
        if (KernelBase) {
    	    BootInfo = KrnGetBootInfo();
    	    HostLibBase->HostIFace = (struct HostInterface *)GetTagData(KRN_HostInterface, 0, BootInfo);
    	    D(bug("[hostlib] HostIFace = 0x%08lX\n", HostLibBase->HostIFace));
    	}
    	CloseLibrary((struct Library *)UtilityBase);
    }
    return (int)HostLibBase->HostIFace;
}

ADD2INITLIB(HostLib_Init, 0)
