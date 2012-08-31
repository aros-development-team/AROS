/*
 * This little utility is specific for Windows-hosted port. It allows you to enter
 * SAD even on completely frozen system.
 * It is written specifically to facilitate finding long-standing lockup bug.
 * Additionally it can serve as a short example showing how to interact with Windows
 * host and use IRQs.
 * The program installs a hook on Windows console, which generates a NMI whenever the
 * user presses Ctrl-Break in the console.
 */

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/kernel.h>

#include <stdio.h>

#ifdef __x86_64__
#define __stdcall __attribute__((ms_abi))
#else
#define __stdcall __attribute__((stdcall))
#endif

struct KernelInterface
{
    int  (*KrnAllocIRQ)(void);
    void (*KrnFreeIRQ)(unsigned char irq);
    void (*KrnCauseIRQ)(unsigned char irq);
    int   *NonMaskableIRQ;
};

static LONG  debugIRQ;
static struct KernelInterface *kernelIf;

static const char *symbols[] =
{
    "KrnAllocIRQ",
    "KrnFreeIRQ",
    "KrnCauseIRQ",
    "NonMaskableInt",
    NULL
};

static void DebugInterrupt(void *a1, void *a2)
{
    Debug(0);
}

/*
 * Be careful! This function is executed by Windows, in console's context!
 * So no AROS calls here!!!
 */
static ULONG __stdcall ConsoleHook(ULONG event)
{
    if (event == 1) /* CTRL_BREAK_EVENT */
    {
        kernelIf->KrnCauseIRQ(debugIRQ);
        return TRUE;
    }
    return FALSE;
}

int __nocommandline = 1;

int main(void)
{
    char *errStr;
    APTR kern32, kernel;
    ULONG unres;
    ULONG __stdcall (*SetConsoleCtrlHandler)(void *HandlerRoutine, ULONG Add);
    APTR KernelBase, HostLibBase;
    APTR intHandle;
    
    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
    {
        printf("Failed to open kernel.resource ???\n");
        return 20;
    }

    HostLibBase = OpenResource("hostlib.resource");
    if (!HostLibBase)
    {
        printf("hostlib.resource not found, likely native system\n");
        return 10;
    }
    
    kern32 = HostLib_Open("kernel32.dll", &errStr);
    if (!kern32)
    {
        printf("kernel32.dll: %s\n", errStr);
        HostLib_FreeErrorStr(errStr);
        return 10;
    }

    SetConsoleCtrlHandler = HostLib_GetPointer(kern32, "SetConsoleCtrlHandler", &errStr);
    if (!SetConsoleCtrlHandler)
    {
        printf("SetConsoleCtrlHandler: %s\n", errStr);
        HostLib_FreeErrorStr(errStr);
        HostLib_Close(kern32, NULL);
        return 10;
    }

    kernel = HostLib_Open("Libs\\Host\\kernel.dll", &errStr);
    if (!kernel)
    {
        printf("kernel.dll: %s\n", errStr);
        HostLib_FreeErrorStr(errStr);
        HostLib_Close(kern32, NULL);
        return 10;
    }

    kernelIf = (struct KernelInterface *)HostLib_GetInterface(kernel, symbols, &unres);
    if ((!kernelIf) || unres)
    {
        printf("Failed to obtain kernel.dll interface, %u symbols unresolved\n", unres);
        
        if (kernelIf)
            HostLib_DropInterface((void **)kernelIf);
        HostLib_Close(kernel, NULL);
        HostLib_Close(kern32, NULL);
        return 10;
    }

    debugIRQ = kernelIf->KrnAllocIRQ();
    if (debugIRQ == -1)
    {
        printf("Failed to create NMI interrupt!\n");
        HostLib_DropInterface((void **)kernelIf);
        HostLib_Close(kernel, NULL);
        HostLib_Close(kern32, NULL);

        return 10;
    }

    intHandle = KrnAddIRQHandler(debugIRQ, DebugInterrupt, NULL, NULL);
    if (!intHandle)
    {
        printf("Failed to add IRQ handler\n");

        HostLib_DropInterface((void **)kernelIf);
        HostLib_Close(kernel, NULL);
        HostLib_Close(kern32, NULL);
        return 10;
    }

    Forbid();
    unres = SetConsoleCtrlHandler(ConsoleHook, TRUE);
    Permit();
    
    if (unres)
    {
        /* Yes, NonMaskable IRQ exists solely for us dirty hackers */
        *kernelIf->NonMaskableIRQ = debugIRQ; /* This turns IRQ into NMI */

        printf("Debug NMI installed, press Ctrl-C to uninstall\n");
        Wait(SIGBREAKF_CTRL_C);

        printf("Done, exiting\n");

        /* Disable NMI, or bad things may happen if someone reuses this IRQ number */
        *kernelIf->NonMaskableIRQ = debugIRQ;
        Forbid();
        SetConsoleCtrlHandler(ConsoleHook, FALSE);
        Permit();
    }
    else
        printf("Failed to install console hook\n");

    KrnRemIRQHandler(intHandle);
    kernelIf->KrnFreeIRQ(debugIRQ);
    HostLib_DropInterface((void **)kernelIf);
    HostLib_Close(kernel, NULL);
    HostLib_Close(kern32, NULL);

    return 0;
}
