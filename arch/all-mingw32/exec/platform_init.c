#include <aros/kernel.h>
#include <proto/arossupport.h>
#include <proto/exec.h>

#include "../kernel/hostinterface.h"
#include "exec_intern.h"

/*
 * Note that we are called very early, so no exec calls here! We don't have
 * ExecBase's functions table yet, only empty data structure!
 * In fact this routine exists only because of need to link up FlushInstructionCache()
 * function before creating vector table. This is done for convenience, what if at some
 * point we are using complete jumps as vectors ? All other functions could be linked
 * in normal init routine. But, well, we do it all in one place.
 */
BOOL Exec_PreparePlatform(struct Exec_PlatformData *pd, struct TagItem *msg)
{
    struct TagItem *tag;
    struct HostInterface *HostIFace;
    void *KernelLib;
    APTR  __stdcall (*GetCurrentProcess)(void);

    tag = LibFindTagItem(KRN_HostInterface, msg);
    if (!tag)
    	return FALSE;

    HostIFace = (struct HostInterface *)tag->ti_Data;

    KernelLib = HostIFace->hostlib_Open("kernel32.dll", NULL);
    if (!KernelLib)
	return FALSE;

    pd->ExitProcess = HostIFace->hostlib_GetPointer(KernelLib, "ExitProcess", NULL);
    if (!pd->ExitProcess)
	return FALSE;

    pd->FlushInstructionCache = HostIFace->hostlib_GetPointer(KernelLib, "FlushInstructionCache", NULL);
    if (!pd->FlushInstructionCache)
	return FALSE;

    GetCurrentProcess = HostIFace->hostlib_GetPointer(KernelLib, "GetCurrentProcess", NULL);
    if (!GetCurrentProcess)
	return FALSE;

    pd->Reboot = HostIFace->Reboot;
    pd->MyProcess = GetCurrentProcess();

    return TRUE;
}
