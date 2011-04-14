#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <proto/hostlib.h>
#include <proto/kernel.h>

#include "../kernel/hostinterface.h"

#include "exec_intern.h"

static const char *libc_symbols[] = {
    "exit",
#ifdef HAVE_SWAPCONTEXT
    "getcontext",
    "makecontext",
    "swapcontext",
#endif
    NULL
};

static int Platform_Init(struct ExecBase *SysBase)
{
    struct TagItem *tag;
    APTR LibCHandle;
    ULONG r;

    /* Fetch restart callback routine from HostInterface */
    tag = LibFindTagItem(KRN_HostInterface, KrnGetBootInfo());
    if (!tag)
    	return FALSE;

    PD(SysBase).Reboot = ((struct HostInterface *)tag->ti_Data)->Reboot;

    HostLibBase = OpenResource("hostlib.resource");
    D(bug("[exec] HostLibBase %p\n", HostLibBase));
    if (!HostLibBase)
	return FALSE;

    /* We use local variable for the handle because we never expunge
       so we will never close it */
    LibCHandle = HostLib_Open(LIBC_NAME, NULL);
    D(bug("[exec] libc handle 0x%p\n", LibCHandle));
    if (!LibCHandle)
	return FALSE;

    PD(SysBase).SysIFace = (struct LibCInterface *)HostLib_GetInterface(LibCHandle, libc_symbols, &r);
    if (PD(SysBase).SysIFace)
    {
	D(bug("[exec] Got libc interface, %u unresolved symbols\n", r));
	if (!r)
	    return TRUE;

	HostLib_DropInterface((APTR *)PD(SysBase).SysIFace);
	HostLib_Close(LibCHandle, NULL);
    }

    return FALSE;
}

ADD2INITLIB(Platform_Init, 0);
