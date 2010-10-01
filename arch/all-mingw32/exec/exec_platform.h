#ifdef __x86_64__
#define __stdcall
#else
#define __stdcall __attribute__((stdcall))
#endif

#define HAVE_PREPAREPLATFORM

struct Exec_PlatformData
{
    void  __stdcall (*ExitProcess)(ULONG uExitCode);
    ULONG __stdcall (*FlushInstructionCache)(APTR hProcess, void *lpBaseAddress, IPTR dwSize);
    LONG  __stdcall (*MessageBox)(APTR hWnd, STRPTR lpText, STRPTR lpCaption, ULONG uType);
    void            (*Reboot)(unsigned char warm);
    APTR  MyProcess;
};
