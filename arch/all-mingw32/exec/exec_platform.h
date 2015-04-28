#ifndef __EXEC_PLATFORM_H
#define __EXEC_PLATFORM_H

#ifdef __x86_64__
#define __stdcall __attribute__((ms_abi))
#else
#define __stdcall __attribute__((stdcall))
#endif

#define HAVE_PREPAREPLATFORM

struct Exec_PlatformData
{
    void  __stdcall (*ExitProcess)(ULONG uExitCode);
    ULONG __stdcall (*FlushInstructionCache)(APTR hProcess, void *lpBaseAddress, IPTR dwSize);
    void            (*Reboot)(unsigned char warm);
    APTR  MyProcess;
};

#define GET_THIS_TASK           (SysBase->ThisTask)
#define SET_THIS_TASK(x)        (SysBase->ThisTask=(x))

#endif /* __EXEC_PLATFORM_H */
