/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hardware/intbits.h>
#include <exec/interrupts.h>
#include <exec/execbase.h>
#include <exec/alerts.h>
#include <proto/exec.h>
#include <aros/asmcall.h>
#include <stdio.h>
#include "sigcore.h"
#define timeval     sys_timeval
#include <sys/time.h>
#undef timeval

#define NOISY	0

void os_disable(void);
static int sig2inttabl[NSIG];
int supervisor;

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
    sighandler (sig, (sigcontext_t *)(&sig+2));
}
#endif /* __FreeBSD__ */

#ifdef __NetBSD__
static void sighandler (int sig, sigcontext_t * sc);
        
static void SIGHANDLER (int sig)
{       
    sighandler (sig, (sigcontext_t *)(&sig+2));
}   
#endif /* __NetBSD__ */

#ifdef __OpenBSD__
static void sighandler (int sig, sigcontext_t * sc);
        
static void SIGHANDLER (int sig)
{       
    sighandler (sig, (sigcontext_t *)(&sig+2));
}   
#endif /* __OpenBSD__ */

#if 0
static void UnixDispatch (sigcontext_t * sc, struct ExecBase * SysBase);
#endif


static void sighandler (int sig, sigcontext_t * sc)
{
    struct IntVector *iv;

    if (supervisor)
    {
#if NOISY
	fprintf (stderr, "Illegal supervisor %d\n", supervisor);
	fflush (stderr);
#endif
	return;
    }

    supervisor++;

    iv=&SysBase->IntVects[sig2inttabl[sig]];
    if (iv->iv_Code)
    {
	AROS_UFC2(void,iv->iv_Code,
	    AROS_UFCA(APTR,iv->iv_Data,A1),
	    AROS_UFCA(struct ExecBase *,SysBase,A6)
	);
    }

    if(SysBase->AttnResched&0x8000)
    {
	SysBase->AttnResched&=~0x8000;

	/* UnixDispatch (sc, SysBase); */
	Dispatch ();
    }

    supervisor--;
} /* sighandler */

#if 0
static void UnixDispatch (sigcontext_t * sc, struct ExecBase * SysBase)
{
    struct Task * this;
    APTR sp;

    /* Get the address of the current task */
    this = FindTask (NULL);

printf ("Dispatch(): Old task=%p (%s)\n",
    this, this && this->tc_Node.ln_Name ? this->tc_Node.ln_Name : "(null)"
);

    /* Save old SP */
    this->tc_SPReg = (APTR)SP(sc);

    /* Switch flag set ? Call user function */
    if (this->tc_Flags & TF_SWITCH)
	(*(this->tc_Switch)) (SysBase);

    /* Save IDNestCnt */
    this->tc_IDNestCnt = SysBase->IDNestCnt;
    SysBase->IDNestCnt = -1;

    /* Get next task from ready list */
    AddTail (&SysBase->TaskReady, (struct Node *)this);
    this = (struct Task *)RemHead (&SysBase->TaskReady);

    /* Set state to RUNNING */
    this->tc_State = TS_RUN;

    /* Restore IDNestCnt */
    SysBase->IDNestCnt = this->tc_IDNestCnt;

    /* Launch flag set ? Call user function */
    if (this->tc_Flags & TF_LAUNCH)
	(*(this->tc_Launch)) (SysBase);

    /* Get new SP */
    sp = this->tc_SPReg;

    /* Check stack pointer */
    if (sp < this->tc_SPLower || sp > this->tc_SPUpper)
    {
	Alert (AT_DeadEnd|AN_StackProbe);
	/* This function never returns */
    }

    SP(sc) = (long)sp;

    if (this->tc_Flags & TF_EXCEPT)
	Exception ();

printf ("Dispatch(): New task=%p (%s)\n",
    this, this && this->tc_Node.ln_Name ? this->tc_Node.ln_Name : "(null)"
);

} /* UnixDispatch */
#endif

void InitCore(void)
{
    static const int sig2int[][2]=
    {
	{   SIGALRM, INTB_VERTB   },
    };
    struct itimerval interval;
    int i;
    struct sigaction sa;

    sa.sa_handler  = (SIGHANDLER_T)SIGHANDLER;
    sigfillset (&sa.sa_mask);
    sa.sa_flags    = SA_RESTART;
#ifdef __linux__
    sa.sa_restorer = NULL;
#endif

    for (i=0; i<(sizeof(sig2int)/sizeof(sig2int[0])); i++)
    {
	sig2inttabl[sig2int[i][0]] = sig2int[i][1];

	sigaction (sig2int[i][0], &sa, NULL);
    }

    interval.it_interval.tv_sec = interval.it_value.tv_sec = 0;
    interval.it_interval.tv_usec = interval.it_value.tv_usec = 1000000/50;

    setitimer (ITIMER_REAL, &interval, NULL);
} /* InitCore */
