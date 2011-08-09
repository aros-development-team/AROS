#include <aros/symbolsets.h>
#include <proto/hostlib.h>

#include "hostinterface.h"
#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"

#define D(x)

/* This comes from kernel_startup.c */
extern struct HostInterface *HostIFace;

static int Alert_Init(struct KernelBase *KernelBase)
{
    APTR ExecHandle;

    /*
     * We use local variable for the handle because we never expunge,
     * so we will never close it
     */
    ExecHandle = HostIFace->hostlib_Open("libAROSBootstrap.so", NULL);
    D(bug("[Alert_Init] Bootstrap handle: 0x%p\n", ExecHandle));
    if (!ExecHandle)
	return FALSE;

    KernelBase->kb_PlatformData->DisplayAlert = HostIFace->hostlib_GetPointer(ExecHandle, "DisplayAlert", NULL);
    D(bug("[Alert_Init] DisplayAlert function: %p\n", KernelBase->kb_PlatformData->DisplayAlert));

    if (KernelBase->kb_PlatformData->DisplayAlert)
	return TRUE;

    HostIFace->hostlib_Close(ExecHandle, NULL);

    return FALSE;
}

ADD2INITLIB(Alert_Init, 15);
