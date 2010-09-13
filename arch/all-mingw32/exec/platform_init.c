#include <proto/exec.h>

#include "../kernel/hostinterface.h"

#include "exec_intern.h"

/*
 * In fact this file can be a temporary thing. Perhaps CPU cache manipulation
 * should be moved to kernel.resource.
 */

extern struct HostInterface *HostIFace;

/*
 * Note that we are called very early, so no exec calls here! We don't have
 * ExecBase's functions table yet, only empty data structure!
 */
BOOL Exec_PreparePlatform(struct ExecBase *SysBase)
{
    void *KernelLib, *UserLib;
    APTR  __stdcall (*GetCurrentProcess)(void);
    struct Exec_PlatformData *pd = &PD(SysBase);

    KernelLib = HostIFace->HostLib_Open("kernel32.dll", NULL);
    if (!KernelLib)
	return FALSE;

    UserLib = HostIFace->HostLib_Open("user32.dll", NULL);
    if (!UserLib)
	return FALSE;

    GetCurrentProcess = HostIFace->HostLib_GetPointer(KernelLib, "GetCurrentProcess", NULL);
    if (!GetCurrentProcess)
        return FALSE;

    pd->FlushInstructionCache = HostIFace->HostLib_GetPointer(KernelLib, "FlushInstructionCache", NULL);
    if (!pd->FlushInstructionCache)
	return FALSE;

    pd->MessageBox = HostIFace->HostLib_GetPointer(UserLib, "MessageBoxA", NULL);
    if (!pd->MessageBox)
	return FALSE;

    pd->MyProcess = GetCurrentProcess();
    return TRUE;
}
