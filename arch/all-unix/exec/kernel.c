/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Initialize the interface to the "hardware".
    Lang: english
*/

#include <exec/types.h>
#include <exec/interrupts.h>
#include <exec/execbase.h>
#include <exec/alerts.h>
#include <aros/asmcall.h>
#include <aros/atomic.h>
#include <aros/debug.h>
#include <aros/config.h>
#include <hardware/intbits.h>

#define timeval sys_timeval
#include <sigcore.h>

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#undef timeval

#include <proto/exec.h>

#define DEBUG_TT    0
#if DEBUG_TT
static struct Task * lastTask;
#endif

/* Don't do any debugging. At 50Hz its far too quick to read anyway :-) */
#define NOISY	0

/* Try and emulate the Amiga hardware interrupts */
static int sig2tab[NSIG];
sigset_t sig_int_mask;	/* Mask of signals that Disable() block */
int intrap;
LONG supervisor;

static BOOL sigactive[NSIG];

/*
    These tables are used to map signals to interrupts
    and trap. There are two tables for the two different kinds
    of events. We also remove all the signals in sig2trap from
    the interrupt disable mask.
*/
static const int sig2int[][2] =
{
    { SIGALRM, INTB_TIMERTICK },
    { SIGUSR1, INTB_SOFTINT },
    { SIGIO,   INTB_DSKBLK }
};

static const int sig2trap[][2] =
{
    { SIGBUS,   2 },
    { SIGSEGV,  3 },
    { SIGILL,   4 },
#ifdef SIGEMT
    { SIGEMT,   13 },
#endif
    { SIGFPE,   13 }
};

/* This is from sigcore.h - it brings in the definition of the
   systems initial signal handler, which simply calls
   sighandler(int signum, sigcontext_t sigcontext)
*/
GLOBAL_SIGNAL_INIT

/* sighandler() Handle the signals:
    You can either turn them into interrupts, or alternatively,
    you can turn them into traps (eg software failures)
*/
static void sighandler(int sig, sigcontext_t * sc)
{
    struct IntVector *iv;

    if(sig == SIGINT)
    {
	exit(0);
    }

#if !AROS_NESTING_SUPERVISOR
    /* Hmm, interrupts are nesting, not a good idea... */
    if(supervisor)
    {
#if NOISY
	fprintf(stderr, "Illegal Supervisor %d\n", supervisor);
	fflush(stderr);
#endif

	return;
    }
#endif

    AROS_ATOMIC_INC(supervisor);

    if (sigactive[sig])
    {
#if NOISY
    	fprintf(stderr,"********* sighandler: sig %d already active **********\n", sig);
	fflush(stderr);
#endif

    	return;
    }
    sigactive[sig] = TRUE;
    
    /* Map the Unix signal to an Amiga signal. */
    iv = &SysBase->IntVects[sig2tab[sig]];

    if (iv->iv_Code)
    {
	/*  Call it. I call with all these parameters for a reason.

	    In my `Amiga ROM Kernel Reference Manual: Libraries and
	    Devices' (the 1.3 version), interrupt servers are called
	    with the following 5 parameters.

	    D1 - Mask of INTENAR and INTREQR
	    A0 - 0xDFF000 (base of custom chips)
	    A1 - Interrupt Data
	    A5 - Interrupt Code vector
	    A6 - SysBase

	    It is quite possible that some code uses all of these, so
	    I must supply them here. Obviously I will dummy some of these
	    though.
	*/
	
	/* If iv->iv_Code calls Disable()/Enable() we could end up
	   having the signals unblocked, which then can cause nesting
	   signals which we do not want. Therefore prevent this from
	   happening by doing this manual Disable()ing/Enable()ing,
	   ie. inc/dec of SysBase->IDNestCnt. */
	   
	AROS_ATOMIC_INC(SysBase->IDNestCnt);
	
	AROS_UFC5(void, iv->iv_Code,
	    AROS_UFCA(ULONG, 0, D1),
	    AROS_UFCA(ULONG, 0, A0),
	    AROS_UFCA(APTR, iv->iv_Data, A1),
	    AROS_UFCA(APTR, iv->iv_Code, A5),
	    AROS_UFCA(struct ExecBase *, SysBase, A6)
	);
	
	/* If this was 100 Hz VBlank timer, emulate 50 Hz VBlank timer if
	   we are on an even 100 Hz tick count */
	   
	if (sig2tab[sig] == INTB_TIMERTICK)
	{
	    static int tick_counter;
	    
	    if ((tick_counter % SysBase->PowerSupplyFrequency) == 0)
	    {
	    	iv = &SysBase->IntVects[INTB_VERTB];
		if (iv->iv_Code)
		{
		    AROS_UFC5(void, iv->iv_Code,
			AROS_UFCA(ULONG, 0, D1),
			AROS_UFCA(ULONG, 0, A0),
			AROS_UFCA(APTR, iv->iv_Data, A1),
			AROS_UFCA(APTR, iv->iv_Code, A5),
			AROS_UFCA(struct ExecBase *, SysBase, A6)
		    );
		}
		
	    }
	    
	    tick_counter++;
	}
	
	AROS_ATOMIC_DEC(SysBase->IDNestCnt);
    }

    /* Has an interrupt told us to dispatch when leaving */
    
#if AROS_NESTING_SUPERVISOR
    if (supervisor == 1)
#endif    
    if (SysBase->AttnResched & 0x8000)
    {
    #if AROS_NESTING_SUPERVISOR
    	// Disable(); commented out, as causes problems with IDNestCnt. Getting completely out of range. 
    #endif
    	AROS_ATOMIC_AND(SysBase->AttnResched, ~0x8000);

	/* Save registers for this task (if there is one...) */
	if (SysBase->ThisTask && SysBase->ThisTask->tc_State != TS_REMOVED)
	    SAVEREGS(SysBase->ThisTask, sc);

	/* Tell exec that we have actually switched tasks... */
	Dispatch ();

	/* Get the registers of the old task */
	RESTOREREGS(SysBase->ThisTask,sc);

	/* Make sure that the state of the interrupts is what the task
	   expects.
	*/
	if (SysBase->IDNestCnt < 0)
	    SC_ENABLE(sc);
	else
	    SC_DISABLE(sc);

	/* Ok, the next step is to either drop back to the new task, or
	    give it its Exception() if it wants one... */

	if (SysBase->ThisTask->tc_Flags & TF_EXCEPT)
	{
	    Disable();
	    Exception();
	    Enable();
	}


#if DEBUG_TT
	if (lastTask != SysBase->ThisTask)
	{
	    fprintf (stderr, "TT %s\n", SysBase->ThisTask->tc_Node.ln_Name);
	    lastTask = SysBase->ThisTask;
	}
#endif

    #if AROS_NESTING_SUPERVISOR
    	// Enable();  commented out, as causes problems with IDNestCnt. Getting completely out of range. 
    #endif	
    }

    /* Leave the interrupt. */

    AROS_ATOMIC_DEC(supervisor);

    sigactive[sig] = FALSE;
    
} /* sighandler */

#if 0
static void traphandler(int sig, sigcontext_t *sc)
{
    int trapNum = sig2tab[sig];
    struct Task *this;

    /* Something VERY bad has happened */
    if( intrap )
    {
	fprintf(stderr, "Double TRAP! Aborting!\n");
	fflush(stderr);
	abort();
    }
    intrap++;

    if( supervisor )
    {
	fprintf(stderr,"Illegal Supervisor %d - Inside TRAP\n", supervisor);
	fflush(stderr);
    }

    /* This is the task that caused the trap... */
    this = SysBase->ThisTask;
    AROS_UFC1(void, this->tc_TrapCode,
	AROS_UFCA(ULONG, trapNum, D0));

    intrap--;
}
#endif

/* Set up the kernel. */
void InitCore(void)
{

    struct itimerval interval;
    int i;
    struct sigaction sa;

    /* We only want signal that we can handle at the moment */
    sigfillset(&sa.sa_mask);
    sigfillset(&sig_int_mask);
    sa.sa_flags = SA_RESTART;
#ifdef __linux__
    sa.sa_restorer = NULL;
#endif

    /* Initialize the translation table */
    bzero(sig2tab, sizeof(sig2tab));
    supervisor = 0;
    intrap = 0;

#if 1
    /* These ones we consider as processor traps */
    //sa.sa_handler = (SIGHANDLER_T)TRAPHANDLER;
    for(i=0; i < (sizeof(sig2trap) / sizeof(sig2trap[0])); i++)
    {
	sig2tab[sig2trap[i][0]] = sig2trap[i][1];
	// sigaction( sig2trap[i][0], &sa, NULL);
	sigdelset( &sig_int_mask, sig2trap[i][0] );
    }
#endif

    /* We want to be signalled - make these interrupts. */
#ifdef SIGCORE_NEED_SA_SIGINFO
    sa.sa_sigaction = (void *)SIGHANDLER;
    sa.sa_flags |= SA_SIGINFO;
#else
    sa.sa_handler = (SIGHANDLER_T)SIGHANDLER;
#endif
    sa.sa_mask = sig_int_mask;
    for(i=0; i < (sizeof(sig2int) / sizeof(sig2int[0])); i++)
    {
	sig2tab[sig2int[i][0]] = sig2int[i][1];
	sigaction( sig2int[i][0], &sa, NULL );
    }
    sigaction( SIGINT, &sa, NULL );

    /* Set up the "pseudo" vertical blank interrupt. */
    interval.it_interval.tv_sec = interval.it_value.tv_sec = 0;
    interval.it_interval.tv_usec =
    interval.it_value.tv_usec = 1000000 / (SysBase->VBlankFrequency * SysBase->PowerSupplyFrequency);

    setitimer(ITIMER_REAL, &interval, NULL);
}
