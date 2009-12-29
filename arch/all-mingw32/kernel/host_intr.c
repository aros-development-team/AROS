#define DEBUG 0

#include <aros/system.h>
#include <excpt.h>
#include <windows.h>
#define __typedef_LONG /* LONG, ULONG, WORD, BYTE and BOOL are declared in Windows headers. Looks like everything  */
#define __typedef_WORD /* is the same except BOOL. It's defined to short on AROS and to int on Windows. This means */
#define __typedef_BYTE /* that you can't use it in OS-native part of the code and can't use any AROS structure     */
#define __typedef_BOOL /* definition that contains BOOL.                                                           */
typedef unsigned AROS_16BIT_TYPE UWORD;
typedef unsigned char UBYTE;

#include <stddef.h>
#include <stdio.h>
#include <exec/alerts.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include "kernel_intern.h"
#include "syscall.h"
#include "host_debug.h"
#include "cpucontext.h"

#define DI(x)   /* Interrupts debug     */
#define DT(x)   /* Traps debug          */
#define DS(x)   /* Task switcher debug  */
#define DIRQ(x) /* IRQ debug		*/

#define AROS_EXCEPTION_SYSCALL 0x80000001

struct SwitcherData
{
    HANDLE MainThread;
    HANDLE IntObjects[INTERRUPTS_NUM];
};

struct ExceptionTranslation
{
    DWORD ExceptionCode;
    unsigned long TrapNum;
};

struct SwitcherData SwData;
DWORD *LastErrorPtr;
unsigned char Ints_Enabled;
unsigned char PendingInts[INTERRUPTS_NUM];
unsigned char Supervisor;
unsigned char Sleep_Mode;
struct ExecBase **SysBasePtr;
struct KernelBase **KernelBasePtr;

void user_handler(uint8_t exception, struct List *list)
{
    if (!IsListEmpty(&list[exception]))
    {
        struct IntrNode *in, *in2;

        ForeachNodeSafe(&list[exception], in, in2)
        {
            if (in->in_Handler)
                in->in_Handler(in->in_HandlerData, in->in_HandlerData2);
        }
    }
}

struct ExceptionTranslation ExceptionsTable[] = {
    {EXCEPTION_ACCESS_VIOLATION     , 2},
    {EXCEPTION_ARRAY_BOUNDS_EXCEEDED, 3},
    {EXCEPTION_BREAKPOINT	    , 4},
    {EXCEPTION_DATATYPE_MISALIGNMENT, 3},
    {EXCEPTION_FLT_DIVIDE_BY_ZERO   , 5},
    {EXCEPTION_GUARD_PAGE	    , 3},
    {EXCEPTION_ILLEGAL_INSTRUCTION  , 4},
    {EXCEPTION_IN_PAGE_ERROR	    , 3},
    {EXCEPTION_INT_DIVIDE_BY_ZERO   , 5},
    {EXCEPTION_PRIV_INSTRUCTION     , 8},
    {EXCEPTION_SINGLE_STEP	    , 9},
    {0				    , 0}
 };

static void core_LeaveInterrupt(void)
{
    struct ExecBase *SysBase = *SysBasePtr;
    
    if (SysBase) {
        if ((char )SysBase->IDNestCnt < 0) {
            core_intr_enable();
	}
    }
}

EXCEPTION_DISPOSITION __declspec(dllexport) core_exception(EXCEPTION_RECORD *ExceptionRecord, void *EstablisherFrame, CONTEXT *ContextRecord, void *DispatcherContext)
{
    	struct ExecBase *SysBase = *SysBasePtr;
    	struct KernelBase *KernelBase = *KernelBasePtr;
	void (*trapHandler)(unsigned long, CONTEXT *) = NULL;
    	REG_SAVE_VAR;

	/* We are already in interrupt and we must not be preempted by task switcher. 
	   Note that up to this point we still can be preempted by task switcher, i
	   hope it's okay. */
        Ints_Enabled = 0;
        /* Enter supervisor mode, */
	Supervisor = 1;
	/* Save important registers that must not be modified */
	CONTEXT_SAVE_REGS(ContextRecord);

	switch (ExceptionRecord->ExceptionCode) {
	case AROS_EXCEPTION_SYSCALL:
	    /* It's a SysCall exception issued by core_syscall() */
	    DI(printf("[KRN] Syscall %lu\n", *ExceptionRecord->ExceptionInformation));
	    switch (*ExceptionRecord->ExceptionInformation)
	    {
	    case SC_CAUSE:
	        core_Cause(SysBase);
	        break;
	    case SC_DISPATCH:
	        core_Dispatch(ContextRecord);
	        break;
	    case SC_SWITCH:
	        core_Switch(ContextRecord);
	        break;
	    case SC_SCHEDULE:
	        core_Schedule(ContextRecord);
	        break;
	    }
	    break;
	default:
	    /* It's something else, likely a CPU trap */
	    printf("[KRN] Exception 0x%08lX, SysBase 0x%p, KernelBase 0x%p\n", ExceptionRecord->ExceptionCode, ContextRecord, SysBase, KernelBase);
	    /* Find out trap handler for caught task */
    	    if (SysBase)
    	    {
        	struct Task *t = SysBase->ThisTask;

        	if (t) {
        	    printf("[KRN] %s 0x%p (%s)\n", t->tc_Node.ln_Type == NT_TASK ? "Task":"Process", t, t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "--unknown--");
		    trapHandler = t->tc_TrapCode;
        	} else {
        	    printf("[KRN] No task\n");
		    trapHandler = SysBase->TaskTrapCode;
		}
    	    }
    	    PRINT_CPUCONTEXT(ContextRecord);

	    DT(printf("Task trap handler 0x%p\n", trapHandler));
	    DT(printf("Exec trap handler 0x%p\n", SysBase->TaskTrapCode));
	    if (trapHandler) {
	    	/* If there is a trap handler, execute it. In fact it's always there, exec.library
		   supplies a default one. But first we have to convert exception code to
		   the well-known */
	        struct ExceptionTranslation *ex;

	        for (ex = ExceptionsTable; ex->ExceptionCode; ex++) {
		    if (ExceptionRecord->ExceptionCode == ex->ExceptionCode)
		        break;
		}
		/* Call our trap handler. Note that we may return, this means that the handler has
		   fixed the problem somehow and we may safely continue */
		DT(printf("Calling trap %u\n", ex->TrapNum));
	        trapHandler(ex->TrapNum, ContextRecord);
	    } else {
	        /* We should never get here. But if we do, it's a true emergency.
		   And we tell Windows to throw us away. */
    	        printf("[KRN] **UNHANDLED EXCEPTION** stopping here...\n");
	        return ExceptionContinueSearch;
	    }
	}

	/* Restore important registers */
	CONTEXT_RESTORE_REGS(ContextRecord);
	/* Exit supervisor */
	Supervisor = 0;
	/* Restore interrupts state. I again hope that being preemted beyond this point is OK */
	core_LeaveInterrupt();
	return ExceptionContinueExecution;
}

DWORD WINAPI TaskSwitcher(struct SwitcherData *args)
{
    HANDLE IntEvent;
    DWORD obj;
    CONTEXT MainCtx;
    REG_SAVE_VAR;
    DS(DWORD res);
    MSG msg;

    for (;;) {
        obj = WaitForMultipleObjects(INTERRUPTS_NUM, args->IntObjects, FALSE, INFINITE);
        DS(bug("[Task switcher] Object %lu signalled\n", obj));
	/* Stop main thread if it's not sleeping */
        if (Sleep_Mode != SLEEP_MODE_ON) {
            DS(res =) SuspendThread(args->MainThread);
    	    DS(bug("[Task switcher] Suspend thread result: %lu\n", res));
	    /* People say that on SMP systems thread is not stopped immediately by SuspendThread().
	       So we have to do our best to ensure that is is really stopped. I hope GetThreadContext()
	       guarantees it. */
	    CONTEXT_INIT_FLAGS(&MainCtx);
    	    DS(res =) GetThreadContext(args->MainThread, &MainCtx);
    	    DS(bug("[Task switcher] Get context result: %lu\n", res));
    	}
	/* Process the interrupt if we are allowed to */
        if (Ints_Enabled) {
    	    Supervisor = 1;
    	    PendingInts[obj] = 0;
    	    /* 
    	     * We get and store the complete CPU context, but set only part of it
	     * because changing some registers causes Windows to immediately shut down
	     * our process. This can be a useful aid for future AROS debuggers.
    	     */
    	    CONTEXT_SAVE_REGS(&MainCtx);
    	    DS(OutputDebugString("[Task switcher] original CPU context: ****\n"));
    	    DS(PrintCPUContext(&MainCtx));
	    /* Call user-defined IRQ handler */
    	    if (*KernelBasePtr)
	    	user_handler(obj, (*KernelBasePtr)->kb_Interrupts);
	    /* Call scheduler */
    	    core_ExitInterrupt(&MainCtx);
	    /* If AROS is going to sleep, set new CPU context */
    	    if (!Sleep_Mode) {
    	        DS(OutputDebugString("[Task switcher] new CPU context: ****\n"));
    	        DS(PrintCPUContext(&MainCtx));
    	        CONTEXT_RESTORE_REGS(&MainCtx);
    	        DS(res =)SetThreadContext(args->MainThread, &MainCtx);
    	        DS(bug("[Task switcher] Set context result: %lu\n", res));
    	    }
	    /* Leave supervisor mode and apply interrupts state */
    	    Supervisor = 0;
	    core_LeaveInterrupt();
    	} else {
	    /* Otherwise remember the interrupt in order to re-submit it later */
    	    PendingInts[obj] = 1;
            DS(bug("[KRN] Interrupts are disabled, interrupt %lu is pending\n", obj));
        }
	/* Resuming main thread if AROS is not sleeping */
        if (Sleep_Mode)
            /* We've entered sleep mode */
            Sleep_Mode = SLEEP_MODE_ON;
        else {
            DS(res =) ResumeThread(args->MainThread);
            DS(bug("[Task switcher] Resume thread result: %lu\n", res));
        }
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
    int i;

    DI(printf("[KRN] enabling interrupts\n"));
    Ints_Enabled = 1;
    /* FIXME: here we do not force timer interrupt, probably this is wrong. However there's no way
       to force-trigger a waitable timer in Windows. A workaround is possible, but the design will
       be complicated then (we need a companion event in this case). Probably it will be implemented
       in future. */
    for (i = INT_IO; i < INTERRUPTS_NUM; i++) {
        if (PendingInts[i]) {
            DI(printf("[KRN] enable: sigalling about pending interrupt %lu\n", i));
            SetEvent(SwData.IntObjects[i]);
        }
    }
}

void __declspec(dllexport) core_syscall(unsigned long n)
{
    RaiseException(AROS_EXCEPTION_SYSCALL, 0, 1, &n);
    /* If after RaiseException we are still here, but Sleep_Mode != 0, this likely means
       we've just called SC_SCHEDULE, SC_SWITCH or SC_DISPATCH, and it is putting us to sleep.
       Sleep mode will be committed as soon as timer IRQ happens */
    while(Sleep_Mode) {
    	/* TODO: SwitchToThread() here maybe? But it's dangerous because context switch
    	   will happen inside it and Windows will kill us */
    }
}

unsigned char __declspec(dllexport) core_is_super(void)
{
    return Supervisor;
}

BOOL InitIntObjects(HANDLE *Objs)
{
    int i;

    for (i = 0; i < INTERRUPTS_NUM; i++) {
        Objs[i] = NULL;
        PendingInts[i] = 0;
    }
    /* Timer interrupt is a waitable timer, it's not an event */
    for (i = INT_IO; i < INTERRUPTS_NUM; i++) {
        Objs[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!Objs[i])
            return FALSE;
    }
    return TRUE;
}

void CleanupIntObjects(HANDLE *Objs)
{
    int i;

    for (i = 0; i < INTERRUPTS_NUM; i++) {
        if (Objs[i])
            CloseHandle(Objs[i]);
    }
}

int __declspec(dllexport) core_init(unsigned long TimerPeriod, struct ExecBase **SysBasePointer, struct KernelBase **KernelBasePointer)
{
    HANDLE ThisProcess;
    HANDLE SwitcherThread;
    LARGE_INTEGER VBLPeriod;
    OSVERSIONINFO osver;
    void *MainTEB;
    int i;
    DWORD SwitcherId;
    ULONG LastErrOffset = 0;

    D(printf("[KRN] Setting up interrupts, SysBasePtr = 0x%08lX, KernelBasePtr = 0x%08lX\n", SysBasePointer, KernelBasePointer));
    SysBasePtr = SysBasePointer;
    KernelBasePtr = KernelBasePointer;
    Ints_Enabled = 0;
    Supervisor = 0;
    Sleep_Mode = 0;
    if (InitIntObjects(SwData.IntObjects)) {
    	SwData.IntObjects[INT_TIMER] = CreateWaitableTimer(NULL, 0, NULL);
    	if (SwData.IntObjects[INT_TIMER]) {
	    ThisProcess = GetCurrentProcess();
	    if (DuplicateHandle(ThisProcess, GetCurrentThread(), ThisProcess, &SwData.MainThread, 0, TRUE, DUPLICATE_SAME_ACCESS)) {
	        FillMemory(&osver, sizeof(osver), 0);
	        osver.dwOSVersionInfoSize = sizeof(osver);
	        GetVersionEx(&osver);
	        /* LastError value is part of our context. In order to manipulate it we have to hack
	           into Windows TEB (thread environment block).
	           Since this structure is private, error code offset changes from version to version.
	           The following offsets are known:
	           * Windows 95 and 98 - 0x60
	           * Windows Me - 0x74
	           * Windows NT (all family, fixed at last) - 0x34
	         */
	        switch(osver.dwPlatformId) {
	        case VER_PLATFORM_WIN32_WINDOWS:
	            if (osver.dwMajorVersion == 4) {
	                if (osver.dwMinorVersion > 10)
	                    LastErrOffset = 0x74;
	                else
	                    LastErrOffset = 0x60;
	            }
	            break;
	        case VER_PLATFORM_WIN32_NT:
	            LastErrOffset = 0x34;
	            break;
	        }
	        if (LastErrOffset) {
		    MainTEB = NtCurrentTeb();
		    LastErrorPtr = MainTEB + LastErrOffset;
		    SwitcherThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TaskSwitcher, &SwData, 0, &SwitcherId);
		    if (SwitcherThread) {
			D(printf("[KRN] Task switcher started, ID %lu\n", SwitcherId));
#ifdef SLOW
			TimerPeriod = 5000;
#else
			TimerPeriod = 1000/TimerPeriod;
#endif
			VBLPeriod.QuadPart = -10000*(LONGLONG)TimerPeriod;
			return SetWaitableTimer(SwData.IntObjects[INT_TIMER], &VBLPeriod, TimerPeriod, NULL, NULL, 0);
		    }
			D(else printf("[KRN] Failed to run task switcher thread\n");)
		} else
		    printf("Unsupported Windows version %u.%u, platform ID %u\n", osver.dwMajorVersion, osver.dwMinorVersion, osver.dwPlatformId);
	    }
		D(else printf("[KRN] failed to get thread handle\n");)
	}
	    D(else printf("[KRN] Failed to create timer interrupt\n");)
    }
        D(else printf("[KRN] failed to create interrupt objects\n");)
    CleanupIntObjects(SwData.IntObjects);
    return 0;
}

/*
 * This is the only function to be called by modules other than kernel.resource.
 * It is used for causing interrupts from within asynchronous threads of
 * emul.handler and wingdi.hidd.
 */

unsigned long __declspec(dllexport) KrnCauseIRQ(unsigned char irq)
{
    unsigned long res;

    D(printf("[kernel IRQ] Causing IRQ %u\n", irq));
    res = SetEvent(SwData.IntObjects[irq]);
    D(printf("[kernel IRQ] Result: %ld\n", res));
    return res;
}
