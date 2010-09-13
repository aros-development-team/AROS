#include <proto/exec.h>

#include "../kernel/hostinterface.h"

#include "exec_intern.h"

/*
 * In fact this file can be a temporary thing. Perhaps CPU cache manipulation
 * and system rebooting should also be handled by kernel.resource.
 */

static char *kernel_funcs[] = {
    "ExitProcess",
    "FlushInstructionCache",
    "GetCurrentProcess",
    NULL
};

/*
 * Note that we are called very early, so no exec calls here! We don't have
 * ExecBase's functions table yet, only empty data structure!
 */
BOOL Exec_PreparePlatform(struct ExecBase *SysBase, void *data)
{
    void *KernelLib, *UserLib;
    struct Exec_PlatformData *pd = &PD(SysBase);
    struct HostInterface *HostIFace = data;

    KernelLib = HostIFace->HostLib_Open("kernel32.dll", NULL);
    if (!KernelLib)
	return FALSE;

    UserLib = HostIFace->HostLib_Open("user32.dll", NULL);
    if (!UserLib)
	return FALSE;

    if (HostIFace->HostLib_GetInterface(KernelLib, kernel_funcs, (void **)pd))
        return FALSE;

    pd->MessageBox = HostIFace->HostLib_GetPointer(UserLib, "MessageBoxA", NULL);
    if (!pd->MessageBox)
	return FALSE;

    pd->Reboot = HostIFace->Reboot;
    pd->MyProcess = pd->GetCurrentProcess();
    return TRUE;
}
