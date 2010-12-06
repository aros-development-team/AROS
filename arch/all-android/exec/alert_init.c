#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/hostlib.h>

#include "exec_intern.h"

static int Alert_Init(struct ExecBase *SysBase)
{
    APTR ExecHandle;

    /* We use local variable for the handle because we never expunge
       so we will never close it */
    ExecHandle = HostLib_Open("libAROSBootstrap.so", NULL);
    D(bug("[Alert_Init] Bootstrap handle: 0x%p\n", ExecHandle));
    if (!ExecHandle)
	return FALSE;

    PD(SysBase).DisplayAlert = HostLib_GetPointer(ExecHandle, "DisplayAlert", NULL);
    D(bug("[Alert_Init] DisplayAlert function: %p\n", PD(SysBase).DisplayAlert));
    if (PD(SysBase).DisplayAlert)
	return TRUE;

    HostLib_Close(ExecHandle, NULL);

    return FALSE;
}

ADD2INITLIB(Alert_Init, 10);
