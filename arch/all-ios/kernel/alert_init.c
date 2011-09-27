#include <aros/symbolsets.h>
#include <proto/hostlib.h>

#include "hostinterface.h"
#include "kernel_base.h"
#include "kernel_intern.h"

static int Alert_Init(struct KernelBase *KernelBase)
{
    APTR ExecHandle;

    /*
     * We use local variable for the handle because we never expunge
     * so we will never close it
     */
    ExecHandle = HostLib_Open("Libs/Host/alert.dylib", NULL);
    if (!ExecHandle)
	return FALSE;

    KernelBase->kb_PlatformData->DisplayAlert = HostLib_GetPointer(ExecHandle, "DisplayAlert", NULL);
    if (KernelBase->kb_PlatformData->DisplayAlert)
	return TRUE;

    HostLib_Close(ExecHandle, NULL);

    return FALSE;
}

ADD2INITLIB(Alert_Init, 10);
