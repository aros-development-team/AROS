/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

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

#define WAIT_TIMEOUT 0x00000102

struct KernelInterface
{
    int  (*KrnAllocIRQ)(void);
    void (*KrnFreeIRQ)(unsigned char irq);
    void (*KrnCauseIRQ)(unsigned char irq);
    int   *NonMaskableIRQ;
    int   *Supervisor;
};

struct Kernel32Interface
{
    ULONG __stdcall (*SetConsoleCtrlHandler)(void *HandlerRoutine, ULONG Add);
    APTR  __stdcall (*CreateEvent)(void *lpEventAttributes, ULONG bManualReset, ULONG bInitialState, const char *lpName);
    ULONG __stdcall (*CloseHandle)(APTR hObject);
    ULONG __stdcall (*SetEvent)(APTR hEvent);
    ULONG __stdcall (*WaitForSingleObject)(void *hHandle, ULONG dwMilliseconds);
};

static LONG debugIRQ;
static APTR intAck;
static struct KernelInterface *kernelIf;
static struct Kernel32Interface *winIf;

static const char *symbols[] =
{
    "KrnAllocIRQ",
    "KrnFreeIRQ",
    "KrnCauseIRQ",
    "NonMaskableInt",
    "Supervisor",
    NULL
};

static const char *kern32_symbols[] =
{
    "SetConsoleCtrlHandler",
    "CreateEventA",
    "CloseHandle",
    "SetEvent",
    "WaitForSingleObject",
    NULL
};

static void DebugInterrupt(void *a1, void *a2)
{
    /*
     * Acknowledge the interrupt and call SAD.
     * Acknowledgement mechanism is needed for very heavy lockups, when interrupts
     * become inresponsive. Our console hook will wait for 3 seconds, and if this
     * timeout expores, we consider that our virtualized CPU is dead, and run SAD
     * right from within console hook. Doing this under normal circumstances results
     * in crash because of asynchronous re-entering user-mode code.
     */
    winIf->SetEvent(intAck);
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
        ULONG status;

        kernelIf->KrnCauseIRQ(debugIRQ);
        status = winIf->WaitForSingleObject(intAck, 3000);

        if (status != WAIT_TIMEOUT)
            return TRUE;

        /*
         * This will set supervisor flag. After this it's OK to call kprintf().
         * We add 1 instead of just setting 1 in order to be able to know
         * its original value.
         * Note that here we are running outside of both AROS threads
         * (supervisor and user). It's OK only because if we are here, interrupt
         * thread went defunct, and this is the only thread running.
         */
        *kernelIf->Supervisor += 1;

        kprintf("Timeout waiting for NMI ACK, entering emergency SAD\n");
        Debug(0);
    }
    return FALSE;
}

int __nocommandline = 1;

int main(void)
{
    char *errStr;
    APTR kern32, kernel;
    ULONG unres;
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

    winIf = (struct Kernel32Interface *)HostLib_GetInterface(kern32, kern32_symbols, &unres);
    if ((!winIf) || unres)
    {
        printf("Failed to obtain kernel.dll interface, %u symbols unresolved\n", unres);
        
        if (winIf)
            HostLib_DropInterface((void **)winIf);
        HostLib_Close(kern32, NULL);
        return 10;
    }

    kernel = HostLib_Open("Libs\\Host\\kernel.dll", &errStr);
    if (!kernel)
    {
        printf("kernel.dll: %s\n", errStr);
        HostLib_FreeErrorStr(errStr);
        HostLib_DropInterface((void **)winIf);
        HostLib_Close(kern32, NULL);
        return 10;
    }

    kernelIf = (struct KernelInterface *)HostLib_GetInterface(kernel, symbols, &unres);
    if ((!kernelIf) || unres)
    {
        printf("Failed to obtain kernel.dll interface, %u symbols unresolved\n", unres);
        
        if (kernelIf)
            HostLib_DropInterface((void **)kernelIf);
        HostLib_DropInterface((void **)winIf);
        HostLib_Close(kernel, NULL);
        HostLib_Close(kern32, NULL);
        return 10;
    }

    debugIRQ = kernelIf->KrnAllocIRQ();
    if (debugIRQ == -1)
    {
        printf("Failed to create NMI interrupt!\n");
        HostLib_DropInterface((void **)kernelIf);
        HostLib_DropInterface((void **)winIf);
        HostLib_Close(kernel, NULL);
        HostLib_Close(kern32, NULL);

        return 10;
    }

    intHandle = KrnAddIRQHandler(debugIRQ, DebugInterrupt, NULL, NULL);
    if (!intHandle)
    {
        printf("Failed to add IRQ handler\n");

        HostLib_DropInterface((void **)kernelIf);
        HostLib_DropInterface((void **)winIf);
        HostLib_Close(kernel, NULL);
        HostLib_Close(kern32, NULL);
        return 10;
    }

    Forbid();
    intAck = winIf->CreateEvent(NULL, FALSE, FALSE, NULL);
    if (intAck)
        unres = winIf->SetConsoleCtrlHandler(ConsoleHook, TRUE);
    Permit();

    if (unres)
    {
        /* Yes, NonMaskable IRQ exists solely for us dirty hackers */
        *kernelIf->NonMaskableIRQ = debugIRQ; /* This turns IRQ into NMI */

        printf("Debug NMI#%d installed, press Ctrl-C to uninstall\n", debugIRQ);
        Wait(SIGBREAKF_CTRL_C);

        printf("Done, exiting\n");

        /* Disable NMI, or bad things may happen if someone reuses this IRQ number */
        *kernelIf->NonMaskableIRQ = -1;
    }
    else
        printf("Failed to install console hook\n");

    Forbid();
    if (unres)
        winIf->SetConsoleCtrlHandler(ConsoleHook, FALSE);
    winIf->CloseHandle(intAck);
    Permit();

    KrnRemIRQHandler(intHandle);
    kernelIf->KrnFreeIRQ(debugIRQ);
    HostLib_DropInterface((void **)kernelIf);
    HostLib_DropInterface((void **)winIf);
    HostLib_Close(kernel, NULL);
    HostLib_Close(kern32, NULL);

    return 0;
}
