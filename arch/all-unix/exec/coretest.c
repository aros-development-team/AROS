#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sigcore.h"
#define timeval     sys_timeval
#include <sys/time.h>
#undef timeval

void Dispatch (void);
void Enqueue (struct List * list, struct Node * node);
void AddTail (struct List * list, struct Node * node);
void Switch (void);

ULONG cnt;

#ifdef __linux__
static void sighandler (int sig, sigcontext_t * sc);

static void SIGHANDLER (int sig)
{
    sighandler (sig, (sigcontext_t *)(&sig+1));
}
#endif /* __linux__ */

#ifdef __FreeBSD__
static void sighandler (int sig, sigcontext_t * sc);

static void SIGHANDLER (int sig)
{
    sighandler( sig, (sigcontext_t*)(&sig+2));
}
#endif /* _FreeBSD */

#define SB_SAR 15
#define SF_SAR 0x8000

struct ExecBase _SysBase, * SysBase = &_SysBase;
struct Task Task1, Task2, Task3, TaskMain;

/* List functions */
void AddTail (struct List * list, struct Node * node)
{
    ADDTAIL(list,node);
}

void AddHead (struct List * list, struct Node * node)
{
    ADDHEAD (list, node);
}

void Remove (struct Node * node)
{
    REMOVE(node);
}

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
}

#define STR(x)      (x && x->ln_Name ? (char *)x->ln_Name : "NULL")

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

#define THISTASK	(SysBase->ThisTask)

#define DISABLE 	({sigset_t set; sigfillset(&set); sigprocmask (SIG_BLOCK, &set, NULL);})
#define ENABLE		({sigset_t set; sigfillset(&set); sigprocmask (SIG_UNBLOCK, &set, NULL);})

/* Enable/Disable interrupts */
void Disable (void)
{
    /* Count the calls */
    if (SysBase->IDNestCnt++ < 0)
    {
	DISABLE;
    }
} /* Disable */

void Enable (void)
{
    /* Count the calls */
    if (--SysBase->IDNestCnt < 0)
    {
	ENABLE;
    }
} /* Enable */

/* Allow/forbid task switches */
void Forbid (void)
{
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
    AddTail (&SysBase->TaskReady, (struct Node *)task);
}

/* Switch to a new task if the current task is not running and no
    exception has been raised. */
void Switch (void)
{
    struct Task * task = THISTASK;

    /* Check that the task is not running and no exception is pending */
    if (task->tc_State != TS_RUN && !(task->tc_Flags & TF_EXCEPT) )
    {
	/* Reset the counter */
	SysBase->IDNestCnt = 0;

	/* Allow signals */
	ENABLE;

	/* make sure there is a signal */
	kill (getpid(), SIGALRM);
    }
}

/* This waits for a signal (it's a dummy right now. All it does is
    switch to another task). */
ULONG Wait (ULONG sigmask)
{
    struct Task * task = THISTASK;

    /* Task is no longer running */
    task->tc_State = TS_READY;

    /* Let another task run */
    Switch ();

    /* When I get the CPU back, this code is executed */
    return 0;
}

/* Simple main for a task: Print a message and wait for a signal */
void Main1 (void)
{
    struct Task * task = THISTASK;
    STRPTR name = task->tc_Node.ln_Name;

    while (1)
    {
	printf ("Main1: %s\n", name);

	Wait (0);
    }
}

/* Another method of waiting (but a worse one) */
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

	busy_wait();
    }
}

#define DEBUG_STACK	0

/* The signal handler. It will store the current tasks context and
    switch to another task if this is allowed right now. */
static void sighandler (int sig, sigcontext_t * sc)
{
    SP_TYPE * SP;

    cnt ++;

    /* Are task switches allowed ? */
    if (SysBase->TDNestCnt < 0)
    {
	/* Save all registers and the stack pointer */
	SAVEREGS(SP,sc);

#if DEBUG_STACK
	PRINT_SC(sc);
	PRINT_STACK(SP);
#endif

	THISTASK->tc_SPReg = SP;

	/* Find a new task to run */
	Dispatch ();

	/* Restore stack pointer and registers of new task */
	SP = THISTASK->tc_SPReg;

#if DEBUG_STACK
	PRINT_STACK(SP);
	printf ("\n");
#endif

	RESTOREREGS(SP,sc);
    }
    else
    {
	/* Set flag: switch tasks as soon as switches are allowed again */
	SysBase->SysFlags |= SF_SAR;
    }

} /* sighandler */

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
	printf ("illegal stack\n");
    }

    /* Try to find a task which is ready to run */
    if ((task = GetHead (&SysBase->TaskReady)))
    {
#if 1
printf ("Dispatch: Old = %s (Stack = %ld), new = %s\n",
    this->tc_Node.ln_Name,
    (IPTR)this->tc_SPUpper - (IPTR)this->tc_SPReg,
    task->tc_Node.ln_Name);
#endif

	/* Remove new task from the list */
	Remove ((struct Node *)task);

	/* Sort the old task into the list of tasks which want to run */
	Reschedule (this);

#if 0 /* TODO this doesn't work, yet */
	/* Save disable counters */
	this->tc_TDNestCnt = SysBase->TDNestCnt;
	this->tc_IDNestCnt = SysBase->IDNestCnt;

	/* Set new counters */
	SysBase->TDNestCnt = task->tc_TDNestCnt;
	SysBase->IDNestCnt = task->tc_IDNestCnt;
#endif

	/* Switch task */
	THISTASK = task;

	/* Set new states of the tasks */
	this->tc_State = TS_READY;
	task->tc_State = TS_RUN;
    }
} /* Dispatch */

/* Initialize the system: Install an interrupt handler and make sure
    it is called at 50Hz */
void InitCore(void)
{
    struct sigaction sa;
    struct itimerval interval;

    /* Install a handler for the ALARM signal */
    sa.sa_handler  = (SIGHANDLER_T)SIGHANDLER;
    sa.sa_flags    = SA_RESTART;
#ifdef __linux__
    sa.sa_restorer = NULL;
#endif /* __linux__ */
    sigfillset (&sa.sa_mask);

    sigaction (SIGALRM, &sa, NULL);

    /* Set 50Hz intervall for ALARM signal */
    interval.it_interval.tv_sec  = interval.it_value.tv_sec  = 0;
    interval.it_interval.tv_usec = interval.it_value.tv_usec = 1000000/50;

    setitimer (ITIMER_REAL, &interval, NULL);
} /* InitCore */

/* Create a new task */
void AddTask (struct Task * task, STRPTR name, BYTE pri, APTR pc)
{
    SP_TYPE * sp;

    memset (task, 0, sizeof (struct Task));

    task->tc_Node.ln_Pri = pri;
    task->tc_Node.ln_Name = name;
    task->tc_State = TS_READY;
    sp = malloc (4096 * sizeof (long));
    task->tc_SPLower = sp;
    sp += 4096;
    task->tc_SPUpper = sp;
    PREPARE_INITIAL_FRAME(sp,pc);
    task->tc_SPReg = sp;

    Enqueue (&SysBase->TaskReady, (struct Node *)task);
}

/*
    Main routine: Create four tasks (three with the Mains above and one
    for main(). Wait for some task switches then terminate cleanly.
*/
int main (int argc, char ** argv)
{
    /* Init SysBase */
    NEWLIST (&SysBase->TaskReady);
    NEWLIST (&SysBase->TaskWait);
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
    */
    TaskMain.tc_Node.ln_Pri = 0;
    TaskMain.tc_Node.ln_Name = "Main";
    TaskMain.tc_State = TS_READY;
    TaskMain.tc_SPLower = 0;
    TaskMain.tc_SPUpper = &argc;

    THISTASK = &TaskMain;

    /* Start interrupts and allow them. */
    InitCore ();
    Enable ();
    Permit ();

    /* Wait for 10000 signals */
    while (cnt < 10000)
    {
	printf ("%6ld\n", cnt);
	busy_wait();
    }

    /* Make sure we don't get disturbed in the cleanup */
    Disable ();

    /* Show how many signals have been processed */
    printf ("Exit %ld\n", cnt);

    return 0;
} /* main */

