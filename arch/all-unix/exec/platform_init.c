#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/hostlib.h>

#include "../kernel/hostinterface.h"

#include "exec_intern.h"

static const char *libc_symbols[] = {
    "getcontext",
    "makecontext",
    "swapcontext",
    "exit",
    NULL
};

BOOL Exec_PreparePlatform(struct Exec_PlatformData *pd, struct HostInterface *HostIFace)
{
    pd->Reboot = HostIFace->Reboot;
    return TRUE;
}

static int Platform_Init(struct ExecBase *SysBase)
{
    APTR HostLibBase;
    APTR LibCHandle;
    ULONG r;

    HostLibBase = OpenResource("hostlib.resource");
    D(bug("[exec] HostLibBase %p\n", HostLibBase));
    if (!HostLibBase)
	return FALSE;

    /* We use local variable for the handle because we never expunge
       so we will never close it */
    LibCHandle = HostLib_Open(LIBC_NAME, NULL);
    if (!LibCHandle)
	return FALSE;

    PD(SysBase).SysIFace = (struct LibCInterface *)HostLib_GetInterface(LibCHandle, libc_symbols, &r);
    if (PD(SysBase).SysIFace)
    {
	if (!r)
	    return TRUE;

	HostLib_DropInterface((APTR *)PD(SysBase).SysIFace);
	HostLib_Close(LibCHandle, NULL);
    }

    return FALSE;
}

ADD2INITLIB(Platform_Init, 0);
