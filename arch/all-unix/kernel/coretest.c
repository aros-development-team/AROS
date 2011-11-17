/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/lists.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel_cpu.h>
#include <sys/time.h>

/* Prototypes */
void Dispatch (void);
void Enqueue (struct List * list, struct Node * node);
void AddTail (struct List * list, struct Node * node);
void Switch (void);

/* This is used to count the number of interrupts. */
ULONG cnt;

/* This flag means that a task switch is pending */
#define SB_SAR 15
#define SF_SAR 0x8000

/* Dummy SysBase */
struct ExecBase _SysBase;
struct ExecBase* SysBase = &_SysBase;

/* Dummy KernelBase */
struct PlatformData
{
    sigset_t	 sig_int_mask;
};

struct PlatformData Kernel_PlatformData;

#define PD(x) Kernel_PlatformData

/* Some tasks */
struct Task Task1, Task2, Task3, Task4, TaskMain;

/* List functions */
void AddTail (struct List * list, struct Node * node)
{
    ADDTAIL(list,node);
} /* AddTail */

void AddHead (struct List * list, struct Node * node)
{
    ADDHEAD (list, node);
} /* AddHead */

void Remove (struct Node * node)
{
    REMOVE(node);
} /* Remove */

/* Enter a node into a sorted list */
void Enqueue (struct List * list, struct Node * node)
{
    struct Node * next;

    ForeachNode (list, next)
    {
	/* Look for the first node with a lower priority */
	if (node->ln_Pri > next->ln_Pri)
	    break;
    }

    /* Insert "node" before "next" */
    node->ln_Pred	   = next->ln_Pred;
    next->ln_Pred->ln_Succ = node;
    next->ln_Pred	   = node;
    node->ln_Succ	   = next;
} /* Enqueue */

#define STR(x)      (x && x->ln_Name ? (char *)x->ln_Name : "NULL")

/* Print a list with nodes with names. */
void PrintList (struct List * list)
{
    struct Node * node;
    int t = 0;

    printf ("list %p { head=%s, tail=%s } = ", list,
	STR(list->lh_Head), STR(list->lh_TailPred));

    for (node=GetHead(list); node; node=GetSucc(node))
    {
	printf ("%s (%p { succ=%s pred=%s pri=%d}), ", node->ln_Name, node,
	    STR(GetSucc(node)), STR(GetPred(node)),
	    node->ln_Pri
	);
	if (++t > 10)
	    break;
    }

    printf ("\n");
}

#undef STR

/* Macro to get a pointer to the current running task */
#define THISTASK	(SysBase->ThisTask)

/*
    Disable and enable signals. Don't use these in the signal handler
    because then some signal might break the signal handler and then
    stack will overflow.
*/
#define DISABLE 	sigprocmask (SIG_BLOCK, &Kernel_PlatformData.sig_int_mask, NULL)
#define ENABLE		sigprocmask (SIG_UNBLOCK, &Kernel_PlatformData.sig_int_mask, NULL)

/* Enable/Disable interrupts */
void Disable (void)
{
    /*
	Disable signals only if they are not already. The initial value of
	IDNestCnt is -1.
    */
    if (SysBase->IDNestCnt++ < 0)
    {
	DISABLE;
    }
} /* Disable */

void Enable (void)
{
    /*
	Enable signals only if the number of calls of Enable() matches the
	calls of Disable().
    */
    if (--SysBase->IDNestCnt < 0)
    {
	ENABLE;
    }
} /* Enable */

/* Allow/forbid task switches */
void Forbid (void)
{
    /*
	Task switches are only allowed if TDNestCnt is < 0. The initial
	value of TDNestCnt is -1.
    */
    ++ SysBase->TDNestCnt;
} /* Forbid */

void Permit (void)
{
    /* Count calls and check if interrupts are allowed. */
    if (--SysBase->TDNestCnt < 0
	&& SysBase->IDNestCnt < 0
    )
    {
	/*
	    Task switches are allowed again. If a switch is pending
	    right now, do it.
	*/
	if (SysBase->SysFlags & SF_SAR)
	{
	    /* Clear flag */
	    SysBase->SysFlags &= ~SF_SAR;

	    /* Do task switch */
	    Switch ();
	}
    }
} /* Permit */

/* Main routine: Insert a task into the list of tasks to run. */
void Reschedule (struct Task * task)
{
    /*
	The code in here defines how "good" the task switching is.
	There are seveal things which should be taken into account:

	1. No task should block the CPU forever even if it is an
	    endless loop.

	2. Tasks with a higher priority should get the CPU more often.

	3. Tasks with a low priority should get the CPU every now and then.

	Enqueue() fulfills 2 but not 1 and 3. AddTail() fulfills 1 and 3.

	A better way would be to "deteriorate" a task, ie. decrease the
	priority and use Enqueue() but at this time, I can't do this
	because I have no good way to extend the task structure (I
	need a variable to store the original prio).
    */
    AddTail (&SysBase->TaskReady, (struct Node *)task);
} /* Reschedule */

/* Switch to a new task if the current task is not running and no
    exception has been raised. */
ULONG Wait (ULONG sigmask);
void Switch (void)
{
    struct Task * task = THISTASK;

    /* Check that the task is not running and no exception is pending */
    if (task->tc_State != TS_RUN && !(task->tc_Flags & TF_EXCEPT) )
    {
	/* Allow signals. */
	ENABLE;

	/*
	    Make sure there is a signal. This is somewhat tricky: The
	    current task (which is excuting this funcion) will loose the
	    CPU (ie. some code of another task will be executed). Then
	    at some time in the future, the current task (ie. the one
	    that has executed the kill()) will get the CPU back and
	    continue with the code after the kill().
	*/
	kill (getpid(), SIGALRM);
    }
} /* Switch */

/*
    This waits for a "signal". That's not a Unix signal but a flag set
    by some other task (eg. if you send a command to a device, the
    device will call you back when it has processes the command.
    When this happens such a "signal" will be set). The task will be
    suspended until any of the bits given to Wait() are set in the
    tasks' signal mask. Again, this signal mask has nothing to do
    with the Unix signal mask.

    It's a dummy right now. All it does is switch to another task.
*/
ULONG Wait (ULONG sigmask)
{
    struct Task * task = THISTASK;

    /*
	Task is no longer running. If we didn't do this, Switch() would do
	nothing.
    */
    task->tc_State = TS_READY;

    /* Let another task run. */
    Switch ();

    /* When I get the CPU back, this code is executed */
    return 0;
}

/* Simple main for a task: Print a message and wait for a signal. */
void Main1 (void)
{
    struct Task * task = THISTASK;
    STRPTR name = task->tc_Node.ln_Name;

    while (1)
    {
	printf ("Main1: %s\n", name);

	Wait (1);
    }
}

/* Another method of waiting (but an inferior one). */
void busy_wait (void)
{
    int t;

    for (t=cnt; t==cnt; );
}

/* Same as Main1 but wait by polling */
void Main2 (void)
{
    struct Task * task = THISTASK;
    STRPTR name = task->tc_Node.ln_Name;

    while (1)
    {
	printf ("Main2: %s\n", name);

	/*
	    Kids, don't do this at home. I'm a professional.

	    This is to make sure even endless loops don't harm the
	    system. Even if this task has a higher priority than any
	    other task in the system, the other tasks will get the
	    CPU every now and then.
	*/
	busy_wait();
    }
}

#define DEBUG_STACK	0
#define STACKOFFSET	0

/*
    The signal handler. It will store the current tasks context and
    switch to another task if this is allowed right now.
*/
static void sighandler (int sig, regs_t *sc)
{
    cnt ++;

    /* Are task switches allowed ? */
    if (SysBase->TDNestCnt < 0)
    {
        struct AROSCPUContext *ctx = THISTASK->tc_UnionETask.tc_ETask->et_RegFrame;

#if DEBUG_STACK
	PRINT_SC(sc);
#endif

	/* Save all registers and the stack pointer */
	SAVEREGS(ctx, sc);
	THISTASK->tc_SPReg = (APTR)SP(sc);

	/* Find a new task to run */
	Dispatch ();
	/* Restore signal mask */
	if (SysBase->IDNestCnt < 0)
	    SC_ENABLE(sc);
	else
	    SC_DISABLE(sc);

	ctx = THISTASK->tc_UnionETask.tc_ETask->et_RegFrame;
	RESTOREREGS(ctx, sc);
	SP(sc) = (IPTR)THISTASK->tc_SPReg;

#if DEBUG_STACK
	PRINT_SC(sc);
#endif

    }
    else
    {
	/* Set flag: switch tasks as soon as switches are allowed again */
	SysBase->SysFlags |= SF_SAR;
    }
} /* sighandler */

/* Let the sigcore do it's magic */
GLOBAL_SIGNAL_INIT(sighandler)

/* Find another task which is allowed to run and modify SysBase accordingly */
void Dispatch (void)
{
    struct Task * this = THISTASK;
    struct Task * task;

    /* Check the stack */
    if (this->tc_SPReg <= this->tc_SPLower
	|| this->tc_SPReg >= this->tc_SPUpper
    )
    {
	printf ("illegal stack (SP %p, lower %p, upper %p)\n", this->tc_SPReg, this->tc_SPLower, this->tc_SPUpper);
    }

    /* Try to find a task which is ready to run */
    if ((task = (struct Task *)GetHead (&SysBase->TaskReady)))
    {
	printf ("Dispatch: Old = %s (Stack = %lx), new = %s\n",
	this->tc_Node.ln_Name,
	(IPTR)this->tc_SPUpper - (IPTR)this->tc_SPReg,
    	task->tc_Node.ln_Name);

	/* Remove new task from the list */
	Remove ((struct Node *)task);

	/* Sort the old task into the list of tasks which want to run */
	Reschedule (this);

	/* Save disable counters */
	this->tc_TDNestCnt = SysBase->TDNestCnt;
	this->tc_IDNestCnt = SysBase->IDNestCnt;

	/* Set new counters */
	SysBase->TDNestCnt = task->tc_TDNestCnt;
	SysBase->IDNestCnt = task->tc_IDNestCnt;

	/* Switch task */
	THISTASK = task;

	/* Set new states of the tasks */
	this->tc_State = TS_READY;
	task->tc_State = TS_RUN;
    }
    printf("leaving dispatch!\n");
} /* Dispatch */

/*
    Initialize the system: Install an interrupt handler and make sure
    it is called at 50Hz
*/
void InitCore(void)
{
    struct sigaction sa;
    struct itimerval interval;

    /* Install a handler for the ALARM signal */
    sa.sa_handler  = (SIGHANDLER_T)sighandler_gate;
    sa.sa_flags    = SA_RESTART;
#ifdef __linux__
    sa.sa_restorer = NULL;
#endif /* __linux__ */
    sigfillset (&sa.sa_mask);

    sigaction (SIGALRM, &sa, NULL);

    /* Set 50Hz intervall for ALARM signal */
    interval.it_interval.tv_sec  = interval.it_value.tv_sec  = 1;
    interval.it_interval.tv_usec = interval.it_value.tv_usec = 1000000/50;

    setitimer (ITIMER_REAL, &interval, NULL);
} /* InitCore */

#define STACK_SIZE  4096

/* Create a new task */
void AddTask (struct Task * task, STRPTR name, BYTE pri, APTR pc)
{
    IPTR *sp;
    struct AROSCPUContext *ctx;

    /* Init task structure */
    memset (task, 0, sizeof (struct Task));

    /* Init fields with real values */
    task->tc_Node.ln_Pri = pri;
    task->tc_Node.ln_Name = name;
    task->tc_State = TS_READY;

    /* Allow task switches and signals */
    task->tc_TDNestCnt = -1;
    task->tc_IDNestCnt = -1;

    /* Allocate a stack */
    sp = malloc (STACK_SIZE * sizeof(IPTR));

    /*
	Copy bounds of stack in task structure. Note that the stack
	grows to small addresses (ie. storing something on the stack
	decreases the stack pointer).
    */
    task->tc_SPLower = sp;
    sp += STACK_SIZE;
    task->tc_SPUpper = sp;

    /*
	Let the sigcore do it's magic. Create a frame from which an
	initial task context can be restored from.
    */
 
    task->tc_UnionETask.tc_ETask = malloc(sizeof(struct IntETask));
    task->tc_Flags |= TF_ETASK;
    ctx = malloc(sizeof(struct AROSCPUContext));
    task->tc_UnionETask.tc_ETask->et_RegFrame = ctx;

    PREPARE_INITIAL_CONTEXT(ctx);
    PREPARE_INITIAL_FRAME(ctx, sp, (IPTR)pc);

    /* Save new stack pointer */
    task->tc_SPReg = sp;

    /* Add task to queue by priority */
    Enqueue (&SysBase->TaskReady, (struct Node *)task);
} /* AddTask */

/*
    Main routine: Create four tasks (three with the Mains above and one
    for main(). Wait for some task switches then terminate cleanly.
*/
int main (int argc, char ** argv)
{
    /* Init SysBase */
    NEWLIST (&SysBase->TaskReady);
    NEWLIST (&SysBase->TaskWait);

    sigfillset(&Kernel_PlatformData.sig_int_mask);

    /* Signals and task switches are not allowed right now */
    SysBase->IDNestCnt = 0;
    SysBase->TDNestCnt = 0;

    /* Add three tasks */
    AddTask (&Task1, "Task 1", 0, Main1);
    AddTask (&Task2, "Task 2", 5, Main2);
    AddTask (&Task3, "Task 3", 0, Main2);
    PrintList (&SysBase->TaskReady);

    /*
	Add main task. Make sure the stack check is ok. This task is *not*
	added to the list. It is stored in THISTASK and will be added to
	the list at the next call to Dispatch().

	Also a trick with the stack: This is the stack of the Unix process.
	We don't know where it lies in memory nor how big it is (it can
	grow), so we set the maximum possible limits.
    */
    TaskMain.tc_Node.ln_Pri = 0;
    TaskMain.tc_Node.ln_Name = "Main";
    TaskMain.tc_State = TS_READY;
    TaskMain.tc_SPLower = 0;
    TaskMain.tc_SPUpper = (APTR)-1;

    TaskMain.tc_UnionETask.tc_ETask = malloc(sizeof(struct IntETask));
    TaskMain.tc_Flags |= TF_ETASK;
    TaskMain.tc_UnionETask.tc_ETask->et_RegFrame = malloc(sizeof(struct AROSCPUContext));

    /* The currently running task is me, myself and I */
    THISTASK = &TaskMain;

    /* Start interrupts and allow them. */
    InitCore ();
    Enable ();
    Permit ();

    /* Wait for 10000 signals */
    while (cnt < 20)
    {
	printf ("%6d\n", cnt);

	/* Wait for a "signal" from another task. */
	Wait (1);
    }

    /* Make sure we don't get disturbed in the cleanup */
    Disable ();

    /* Show how many signals have been processed */
    printf ("Exit after %d signals\n", cnt);

    return 0;
} /* main */

