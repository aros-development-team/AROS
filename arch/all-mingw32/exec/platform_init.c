/*
 * In fact this file can be a temporary thing. Perhaps CPU cache manipulation
 * and system rebooting should also be handled by kernel.resource.
 */

#include <proto/exec.h>

#include "../kernel/hostinterface.h"
#include "exec_intern.h"

/*
 * Note that we are called very early, so no exec calls here! We don't have
 * ExecBase's functions table yet, only empty data structure!
 */
BOOL Exec_PreparePlatform(struct Exec_PlatformData *pd, struct HostInterface *HostIFace)
{
    void *KernelLib, *UserLib;
    APTR  __stdcall (*GetCurrentProcess)(void);

    KernelLib = HostIFace->HostLib_Open("kernel32.dll", NULL);
    if (!KernelLib)
	return FALSE;

    UserLib = HostIFace->HostLib_Open("user32.dll", NULL);
    if (!UserLib)
	return FALSE;

    pd->ExitProcess = HostIFace->HostLib_GetPointer(KernelLib, "ExitProcess", NULL);
    if (!pd->ExitProcess)
	return FALSE;

    pd->FlushInstructionCache = HostIFace->HostLib_GetPointer(KernelLib, "FlushInstructionCache", NULL);
    if (!pd->FlushInstructionCache)
	return FALSE;

    pd->MessageBox = HostIFace->HostLib_GetPointer(UserLib, "MessageBoxA", NULL);
    if (!pd->MessageBox)
	return FALSE;

    GetCurrentProcess = HostIFace->HostLib_GetPointer(KernelLib, "GetCurrentProcess", NULL);
    if (!GetCurrentProcess)
	return FALSE;

    pd->Reboot = HostIFace->Reboot;
    pd->MyProcess = GetCurrentProcess();

    return TRUE;
}
