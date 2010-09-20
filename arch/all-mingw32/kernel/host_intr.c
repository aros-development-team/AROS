#include <stddef.h>
#include <stdio.h>
#include <windows.h>

/* PRINT_CPUCONTEXT() macro uses bug() because it can be useful on AROS side too. */
#define bug printf

#include "host_intern.h"
#include "host_irq.h"
#include "kernel_cpu.h"

/*
 * Console output in Windows seems to be protected via semaphores. SuspendThread() at the moment
 * of writing something to console leaves it in locked state. Attempt to write something to it from
 * within task scheduler may halt the system. Because of this only init debug actually works and
 * is safe to use.
 */
#define D(x)	 /* Init debug		*/
#define DI(x)    /* Interrupts debug    */
#define DS(x)    /* Task switcher debug */

HANDLE MainThread;
DWORD MainThreadId;
unsigned char Ints_Enabled;
unsigned short Ints_Num;
HANDLE IntObjects[256];
unsigned char PendingInts[256];
unsigned char AllocatedInts[256];
char *IDNestCnt;

/* Virtual CPU control registers */
int           __declspec(dllexport) (*TrapVector)(unsigned int num, ULONG_PTR *args, CONTEXT *regs);
void          __declspec(dllexport) (*IRQVector)(unsigned int num, CONTEXT *regs);
unsigned char __declspec(dllexport) Supervisor;
unsigned char __declspec(dllexport) Sleep_Mode;
DWORD *       __declspec(dllexport) LastErrorPtr;

void core_intr_enable(void);

static inline void core_LeaveInterrupt(void)
{
    if (*IDNestCnt < 0)
        core_intr_enable();
}

LONG WINAPI exceptionHandler(EXCEPTION_POINTERS *exptr)
{
    DWORD ExceptionCode = exptr->ExceptionRecord->ExceptionCode;
    CONTEXT *ContextRecord = exptr->ContextRecord;
    DWORD ThreadId;
    REG_SAVE_VAR;

    /* We are already in interrupt and we must not be preempted by task switcher. 
       Note that up to this point we still can be preempted by task switcher, i
       really hope it's okay. */
    Ints_Enabled = 0;

    /* Exception in other thread, probably in virtual hardware. Die. */
    ThreadId = GetCurrentThreadId();
    if (ThreadId != MainThreadId)
    {
	printf("[KRN] Service thread 0x%lu, exception 0x%08lX\n", ThreadId, ExceptionCode);
	PRINT_CPUCONTEXT(ContextRecord);

	return EXCEPTION_CONTINUE_SEARCH;
    }

    /* Enter supervisor mode. We can already be in supervisor (crashed inside
       IRQ handler), so we increment in order to retain previous state. */
    Supervisor++;
    /* Save important registers that must not be modified */
    CONTEXT_SAVE_REGS(ContextRecord);

    /* Call trap handler */
    if (TrapVector(ExceptionCode, exptr->ExceptionRecord->ExceptionInformation, ContextRecord))
    {
        printf("[KRN] **UNHANDLED EXCEPTION 0x%08lX** stopping here...\n", ExceptionCode);

	return EXCEPTION_CONTINUE_SEARCH;
    }

    /* Restore important registers */
    CONTEXT_RESTORE_REGS(ContextRecord);
    /* Exit supervisor */
    Supervisor--;
    core_LeaveInterrupt();

    return EXCEPTION_CONTINUE_EXECUTION;
}

DWORD WINAPI TaskSwitcher()
{
    DWORD obj;
    CONTEXT MainCtx;
    REG_SAVE_VAR;
    DS(DWORD res);

    for (;;)
    {
        obj = WaitForMultipleObjects(Ints_Num, IntObjects, FALSE, INFINITE);
        DS(printf("[Task switcher] Object %lu signalled\n", obj));

	/* Stop main thread if it's not sleeping */
        if (Sleep_Mode != SLEEP_MODE_ON)
	{
            DS(res =) SuspendThread(MainThread);
    	    DS(printf("[Task switcher] Suspend thread result: %lu\n", res));
	    /* People say that on SMP systems thread is not stopped immediately by SuspendThread().
	       So we have to do our best to ensure that is is really stopped. I hope GetThreadContext()
	       guarantees it. */
	    CONTEXT_INIT_FLAGS(&MainCtx);
    	    DS(res =) GetThreadContext(MainThread, &MainCtx);
    	    DS(printf("[Task switcher] Get context result: %lu\n", res));
    	}

	/* Process the interrupt if we are allowed to */
        if (Ints_Enabled)
	{
    	    Supervisor = 1;
    	    PendingInts[obj] = 0;
    	    /* 
    	     * We get and store the complete CPU context, but set only part of it
	     * because changing some registers causes Windows to immediately shut down
	     * our process. This can be a useful aid for future AROS debuggers.
    	     */
    	    CONTEXT_SAVE_REGS(&MainCtx);
    	    DS(printf("[Task switcher] original CPU context: ****\n"));
    	    DS(PRINT_CPUCONTEXT(&MainCtx));

	    /* Call IRQ handler */
	    IRQVector(obj, &MainCtx);

	    /* If AROS is not going to sleep, set new CPU context */
    	    if (Sleep_Mode == SLEEP_MODE_OFF)
	    {
    	        DS(printf("[Task switcher] new CPU context: ****\n"));
    	        DS(PRINT_CPUCONTEXT(&MainCtx));
    	        CONTEXT_RESTORE_REGS(&MainCtx);
    	        DS(res =)SetThreadContext(MainThread, &MainCtx);
    	        DS(printf("[Task switcher] Set context result: %lu\n", res));
    	    }

	    /* Leave supervisor mode */
	    Supervisor = 0;
	    core_LeaveInterrupt();
    	}
	else
	{
	    /* Otherwise remember the interrupt in order to re-submit it later */
    	    PendingInts[obj] = 1;
            DS(printf("[KRN] Interrupts are disabled, interrupt %lu is pending\n", obj));
        }

	/* Resuming main thread if AROS is not sleeping */
        if (Sleep_Mode == SLEEP_MODE_OFF)
	{
	    DS(printf("[Task switcher] Resuming main thread\n"));
            DS(res =) ResumeThread(MainThread);
            DS(printf("[Task switcher] Resume thread result: %lu\n", res));
        }
	else
	    /* We've entered sleep mode */
            Sleep_Mode = SLEEP_MODE_ON;

	
    }
    return 0;
}

/* ****** Interface functions ****** */

void __declspec(dllexport) core_intr_disable(void)
{
    DI(printf("[KRN] disabling interrupts\n"));
    Ints_Enabled = 0;
}

void __declspec(dllexport) core_intr_enable(void)
{
    unsigned char i;

    /* If we are in supervisor mode, don't do anything. Interrupts will
       be enabled upon leaving supervisor mode by core_LeaveInterrupt().
       Otherwise we can end up in nested interrupts */
    if (Supervisor)
	return;

    DI(printf("[KRN] enabling interrupts\n"));
    Ints_Enabled = 1;
    /* FIXME: here we do not force timer interrupt, probably this is wrong. However there's no way
       to force-trigger a waitable timer in Windows. A workaround is possible, but the design will
       be complicated then (we need a companion event in this case). Probably it will be implemented
       in future. */
    for (i = 1; i < Ints_Num; i++) {
        if (PendingInts[i]) {
            DI(printf("[KRN] enable: sigalling about pending interrupt %lu\n", i));
            SetEvent(IntObjects[i]);
        }
    }
}

void __declspec(dllexport) core_raise(DWORD code, const ULONG_PTR n)
{
    /* This ensures that we are never preempted inside RaiseException().
       Upon exit from the syscall interrupt state will be restored by
       core_LeaveInterrupt() */
    Ints_Enabled = 0;
    RaiseException(code, 0, 1, &n);

    /* If after RaiseException we are still here, but Sleep_Mode != 0, this likely means
       we've just called SC_SCHEDULE, SC_SWITCH or SC_DISPATCH, and it is putting us to sleep.
       Sleep mode will be committed as soon as timer IRQ happens */
    while (Sleep_Mode);
}

int __declspec(dllexport) core_init(unsigned int TimerPeriod, char *idnestcnt)
{
    HANDLE ThisProcess;
    HANDLE SwitcherThread;
    LARGE_INTEGER VBLPeriod;
    void *MainTEB;
    int i;
    DWORD SwitcherId;

    D(printf("[KRN] Setting up interrupts, IDNestCnt = 0x%p\n", idnestcnt));
    IDNestCnt = idnestcnt;
    Ints_Enabled = 0;
    Supervisor = 0;
    Sleep_Mode = 0;
    for (i = 1; i < 256; i++)
    {
	IntObjects[i] = NULL;
        AllocatedInts[i] = 0;
    }

    /* Set up traps */
    MainThreadId = GetCurrentThreadId();
    SetUnhandledExceptionFilter(exceptionHandler);

    /* Set up debug I/O */
    conin  = GetStdHandle(STD_INPUT_HANDLE);
    conout = GetStdHandle(STD_OUTPUT_HANDLE);

    /* Statically allocate main system timer */
    IntObjects[INT_TIMER] = CreateWaitableTimer(NULL, 0, NULL);
    if (!IntObjects[INT_TIMER])
    {
	D(printf("[KRN] Failed to create timer interrupt\n");)
	return FALSE;
    }
    AllocatedInts[INT_TIMER] = 1;
    PendingInts[INT_TIMER] = 0;
    Ints_Num = 1;

    ThisProcess = GetCurrentProcess();
    if (DuplicateHandle(ThisProcess, GetCurrentThread(), ThisProcess, &MainThread, 0, TRUE, DUPLICATE_SAME_ACCESS))
    {
#ifdef __x86_64__
	#define LastErrOffset 0x34
#else
	/* On 32-bit x86 we have to figure out the offset depending on Windows version */
	OSVERSIONINFO osver = {0};
	ULONG LastErrOffset = 0;

	osver.dwOSVersionInfoSize = sizeof(osver);
	GetVersionEx(&osver);

	/*
	 * LastError value is part of our context. In order to manipulate it we have to hack
	 * into Windows TEB (thread environment block).
	 * Since this structure is private, error code offset changes from version to version.
	 * The following offsets are known:
	 * - Windows 95 and 98 - 0x60
	 * - Windows Me - 0x74
	 * - Windows NT (all family, fixed at last) - 0x34
	 */
	switch(osver.dwPlatformId)
	{
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

	if (!LastErrOffset)
	{
	    printf("Unsupported Windows version %lu.%lu, platform ID %lu\n", osver.dwMajorVersion, osver.dwMinorVersion, osver.dwPlatformId);
	    return FALSE;
	}
#endif

	MainTEB = NtCurrentTeb();
	LastErrorPtr = MainTEB + LastErrOffset;

	SwitcherThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TaskSwitcher, NULL, 0, &SwitcherId);
	if (SwitcherThread)
	{
	    D(printf("[KRN] Task switcher started, ID %lu\n", SwitcherId));
#ifdef SLOW
	    TimerPeriod = 5000;
#else
	    TimerPeriod = 1000/TimerPeriod;
#endif
	    VBLPeriod.QuadPart = -10000*(LONGLONG)TimerPeriod;
	    return SetWaitableTimer(IntObjects[INT_TIMER], &VBLPeriod, TimerPeriod, NULL, NULL, 0);
	}
	    D(else printf("[KRN] Failed to run task switcher thread\n");)
    }
	D(else printf("[KRN] failed to get thread handle\n");)

    CloseHandle(IntObjects[INT_TIMER]);
    return FALSE;
}

/*
 * The following is host-side IRQ API.
 *
 * It is used by virtual hadrware implemented as asynchronous host operating
 * system threads.
 *
 */

long __declspec(dllexport) KrnAllocIRQ(void)
{
    long irq = 0;
    
    for (irq = 0; irq < 256; irq++) {
        if (!AllocatedInts[irq]) {
	    if (!IntObjects[irq]) {
	        IntObjects[irq] = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (!IntObjects[irq])
		    return -1;
	    }
	    PendingInts[irq] = 0;
	    AllocatedInts[irq] = 1;
	    if (irq == Ints_Num)
		Ints_Num++;
	    return irq;
	}
    }
    return -1;
}

void __declspec(dllexport) KrnFreeIRQ(unsigned char irq)
{
    AllocatedInts[irq] = 0;
    while (!AllocatedInts[Ints_Num - 1])
        Ints_Num--;
}

void *__declspec(dllexport) KrnGetIRQObject(unsigned char irq)
{
    return IntObjects[irq];
}

unsigned long __declspec(dllexport) KrnCauseIRQ(unsigned char irq)
{
    unsigned long res;

    D(printf("[kernel IRQ] Causing IRQ %u\n", irq));
    res = SetEvent(IntObjects[irq]);
    D(printf("[kernel IRQ] Result: %ld\n", res));
    return res;
}
