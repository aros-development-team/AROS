#define DEBUG 0

#include <aros/system.h>
#include <windows.h>
#define __typedef_LONG /* LONG, ULONG, WORD, BYTE and BOOL are declared in Windows headers. Looks like everything  */
#define __typedef_WORD /* is the same except BOOL. It's defined to short on AROS and to int on Windows. This means */
#define __typedef_BYTE /* that you can't use it in OS-native part of the code and can't use any AROS structure     */
#define __typedef_BOOL /* definition that contains BOOL.                                                           */
typedef unsigned AROS_16BIT_TYPE UWORD;
typedef unsigned char UBYTE;

#include <stddef.h>
#include <stdio.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include "kernel_intern.h"
#include "syscall.h"
#include "host_debug.h"
#include "contextsave.h"

#define DI(x) /* Interrupts debug     */
#define DS(x) /* Task switcher debug  */

#define AROS_EXCEPTION_SYSCALL 0x80000001

struct SwitcherData {
    HANDLE MainThread;
    HANDLE IntTimer;
};

struct SwitcherData SwData;
unsigned char Ints_Enabled = 0;
struct ExecBase **SysBasePtr;
struct KernelBase **KernelBasePtr;

void user_handler(uint8_t exception)
{
    struct KernelBase *KernelBase = *KernelBasePtr;

    if (KernelBase) {
        if (!IsListEmpty(&KernelBase->kb_Exceptions[exception]))
        {
            struct IntrNode *in, *in2;

            ForeachNodeSafe(&KernelBase->kb_Exceptions[exception], in, in2)
            {
                if (in->in_Handler)
                    in->in_Handler(in->in_HandlerData, in->in_HandlerData2);
            }
        }
    }
    
}

LONG CALLBACK ExceptionHandler(PEXCEPTION_POINTERS Except)
{
    	struct ExecBase *SysBase = *SysBasePtr;
    	struct KernelBase *KernelBase = *KernelBasePtr;
    	REG_SAVE_VAR;

	switch (Except->ExceptionRecord->ExceptionCode) {
	case AROS_EXCEPTION_SYSCALL:
	    CONTEXT_SAVE_REGS(Except->ContextRecord);
	    D(printf("[KRN] Syscall exception %lu\n", *Except->ExceptionRecord->ExceptionInformation));
	    switch (*Except->ExceptionRecord->ExceptionInformation)
	    {
	    case SC_CAUSE:
	        if (SysBase)
	            core_Cause(SysBase);
	        break;
	    case SC_DISPATCH:
	        core_Dispatch(Except->ContextRecord);
	        break;
	    case SC_SWITCH:
	        core_Switch(Except->ContextRecord);
	        break;
	    case SC_SCHEDULE:
	        core_Schedule(Except->ContextRecord);
	        break;
	    }
	    CONTEXT_RESTORE_REGS(Except->ContextRecord);
	    return EXCEPTION_CONTINUE_EXECUTION;
	default:
	    printf("[KRN] Exception 0x%08lX handler. Context @ %p, SysBase @ %p, KernelBase @ %p\n", Except->ExceptionRecord->ExceptionCode, Except->ContextRecord, SysBase, KernelBase);
    	    if (SysBase)
    	    {
        	struct Task *t = SysBase->ThisTask;
        	printf("[KRN] %s %p (%s)\n", t->tc_Node.ln_Type == NT_TASK ? "Task":"Process", t, t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "--unknown--");
    	    }
	    printf("[KRN] **UNHANDLED EXCEPTION** stopping here...\n");
	    return EXCEPTION_EXECUTE_HANDLER;
	}
}

DWORD TaskSwitcher(struct SwitcherData *args)
{
    HANDLE IntEvent;
    DWORD obj;
    CONTEXT MainCtx;
    REG_SAVE_VAR;
    DS(DWORD res);

    for (;;) {
        WaitForSingleObject(args->IntTimer, INFINITE);
        DS(OutputDebugString("[Task switcher] Timer interrupt\n"));
    	DS(res =) SuspendThread(args->MainThread);
    	DS(bug("[Task switcher] Suspend thread result: %lu\n", res));
    	if (Ints_Enabled) {
    	    user_handler(0);

    	    /* 
    	     * We will get and store the complete CPU context, but set only part of it.
    	     * This can be a useful aid for future AROS debuggers.
    	     */
    	    CONTEXT_INIT_FLAGS(&MainCtx);
    	    DS(res =) GetThreadContext(args->MainThread, &MainCtx);
    	    DS(bug("[Task switcher] Get context result: %lu\n", res));
    	    CONTEXT_SAVE_REGS(&MainCtx);
    	    DS(OutputDebugString("[Task switcher] original CPU context: ****\n"));
    	    DS(PrintCPUContext(&MainCtx));
    	    core_ExitInterrupt(&MainCtx);
    	    DS(OutputDebugString("[Task switcher] new CPU context: ****\n"));
    	    DS(PrintCPUContext(&MainCtx));
    	    CONTEXT_RESTORE_REGS(&MainCtx);
    	    DS(res =)SetThreadContext(args->MainThread, &MainCtx);
    	    DS(bug("[Task switcher] Set context result: %lu\n", res));
    	}
            DS(else printf("[KRN] Interrupts are disabled\n"));
        DS(res =) ResumeThread(args->MainThread);
        DS(bug("[Task switcher] Resume thread result: %lu\n", res));
    }
    return 0;
}

/* ****** Interface functions ****** */

long __declspec(dllexport) core_intr_disable(void)
{
    DI(printf("[KRN] disabling interrupts\n"));
    Ints_Enabled = 0;
}

long __declspec(dllexport) core_intr_enable(void)
{
    DI(printf("[KRN] enabling interrupts\n"));
    Ints_Enabled = 1;
}

void __declspec(dllexport) core_syscall(unsigned long n)
{
    RaiseException(AROS_EXCEPTION_SYSCALL, 0, 1, &n);
}

int __declspec(dllexport) core_init(unsigned long TimerPeriod, struct ExecBase **SysBasePointer, struct KernelBase **KernelBasePointer)
{
    HANDLE ThisProcess;
    HANDLE SwitcherThread;
    DWORD SwitcherId;
    LARGE_INTEGER VBLPeriod;

    D(printf("[KRN] Setting up interrupts, SysBasePtr = 0x%08lX, KernelBasePtr = 0x%08lX\n", SysBasePointer, KernelBasePointer));
    SysBasePtr = SysBasePointer;
    KernelBasePtr = KernelBasePointer;
    SetUnhandledExceptionFilter(ExceptionHandler);
    SwData.IntTimer = CreateWaitableTimer(NULL, 0, NULL);
    if (SwData.IntTimer) {
	ThisProcess = GetCurrentProcess();
	if (DuplicateHandle(ThisProcess, GetCurrentThread(), ThisProcess, &SwData.MainThread, 0, TRUE, DUPLICATE_SAME_ACCESS)) {
	    SwitcherThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TaskSwitcher, &SwData, 0, &SwitcherId);
	    if (SwitcherThread) {
	  	D(printf("[KRN] Task switcher started\n"));
	  	CloseHandle(SwitcherThread);
#ifdef SLOW
		TimerPeriod = 5000;
#else
		TimerPeriod = 1000/TimerPeriod;
#endif
		VBLPeriod.QuadPart = -10000*(LONGLONG)TimerPeriod;
	  	return SetWaitableTimer(SwData.IntTimer, &VBLPeriod, TimerPeriod, NULL, NULL, 0);
	    }
	        D(else printf("[KRN] Failed to run task switcher thread\n");)
	}
	    D(else printf("[KRN] failed to get thread handle\n");)
    }
        D(else printf("[KRN] failed to create VBlank timer\n");)
    return 0;
}
