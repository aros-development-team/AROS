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
    int *ptr;

    /*
     * We use local variable for the handle because we never expunge,
     * so we will never close it
     */
    ExecHandle = HostIFace->hostlib_Open("libAROSBootstrap.so", NULL);
    D(bug("[Alert_Init] Bootstrap handle: 0x%p\n", ExecHandle));
    if (!ExecHandle)
	return FALSE;

    ptr = HostIFace->hostlib_GetPointer(ExecHandle, "DisplayPipe", NULL);
    D(bug("[Alert_Init] DisplayPipe pointer: %p\n", ptr));

    KernelBase->kb_PlatformData->alertPipe = ptr ? *ptr : -1;

    HostIFace->hostlib_Close(ExecHandle, NULL);

    return (KernelBase->kb_PlatformData->alertPipe != -1) ? TRUE : FALSE;
}

ADD2INITLIB(Alert_Init, 15);
