/*
 Copyright � 1995-2008, The AROS Development Team. All rights reserved.
 $Id$
 
 Desc: Initialize the interface to the "hardware".
 Lang: english
 */

#include <aros/system.h>
#include <windows.h>
#define __typedef_LONG /* LONG, ULONG, WORD, BYTE and BOOL are declared in Windows headers. Looks like everything  */
#define __typedef_WORD /* is the same except BOOL. It's defined to short on AROS and to int on Windows. This means */
#define __typedef_BYTE /* that you can't use it in OS-native part of the code and can't use any AROS structure     */
#define __typedef_BOOL /* definition that contains BOOL.                                                           */
typedef unsigned AROS_16BIT_TYPE UWORD;
typedef unsigned char UBYTE;

#include <aros/kernel.h>
#include <hardware/intbits.h>
#include <exec/execbase.h>
#include <etask.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include "sigcore.h"

#define ARF_AttnDispatch    (1L<<15)

#define DEBUG_TT    0
#if DEBUG_TT
struct Task * lastTask;
#endif

/* Don't do any debugging. At 50Hz its far too quick to read anyway :-) */
#define NOISY	1

#define Trace(s) {printf(s);printf("\n");fflush(stdout);}

/* Try and emulate the Amiga hardware interrupts */
sigset_t sig_int_mask;	/* Mask of signals that Disable() blocks */
int intrap;
int supervisor;
extern struct ExecBase * SysBase;

/*
 These tables are used to map signals to interrupts
 and trap. There are two tables for the two different kinds
 of events. We also remove all the signals in sig2trap from
 the interrupt disable mask.
 */
#define EVENTS_NUM 3

static const int sig2tab[] =
{
  INTB_SOFTINT,
  INTB_DSKBLK,
  INTB_TIMERTICK
};
  
int sigactive[EVENTS_NUM];

/* TODO: exceptions handling
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
*/
/* On TF_EXCEPT make Exec_Exception being called. */
extern void (*Exec_Exception)(struct ExecBase*);
extern void (*Exec_Dispatch)(struct ExecBase*);

void do_enable()
{
  AROS_ATOMIC_DEC(&SysBase->IDNestCnt);
}

void do_disable()
{
  AROS_ATOMIC_INC(&SysBase->IDNestCnt);
  if (SysBase->IDNestCnt < 0)
  {
	/* If we get here we have big trouble. Someone called
	 1x Disable() and 2x Enable(). IDNestCnt < 0 would
	 mean enable interrupts, but the caller of Disable
	 relies on the function to disable them, so we don¥t
	 do anything here (or maybe a deadend alert?) */
   printf("negative nest count!\n");
  }
}


/* 
 * Handle events.
 * You can either turn them into interrupts, or alternatively,
 * you can turn them into traps (eg software failures)
 */
static void DispatchEvent(DWORD sig, HANDLE sc)
{
  struct IntVector *iv;
  
  if (sig == WAIT_TIMEOUT)
      sig = EVENTS_NUM - 1;
#if NOISY
  fprintf(stderr, "[Task switcher] Event: %lu\n", sig);
  fflush(stderr);
#endif
#if !AROS_NESTING_SUPERVISOR
  /* Hmm, interrupts are nesting, not a good idea... */
  if(supervisor)
  {
#if NOISY
	fprintf(stderr, "[Task switcher] Illegal Supervisor %d\n", supervisor);
	fflush(stderr);
#endif
	return;
  }
#endif
  AROS_ATOMIC_INC(supervisor);
  if (sigactive[sig])
  {
#if NOISY
	fprintf(stderr,"[Task switcher]: event %d already active\n", sig);
	fflush(stderr);
#endif
	
	return;
  }
  sigactive[sig] = 1;
  /* Map the object to an Amiga interrupt. */
  iv = &SysBase->IntVects[sig2tab[sig]];
  
  if (iv->iv_Code)
  {
#if NOISY
	fprintf(stderr,"********* sighandler: calling intvec %p **********\n", iv->iv_Code);
	fflush(stderr);
#endif
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
	
	typedef void (*icall)(unsigned int, unsigned int, void *, void *, void *);
	
#warning direct call to aros, replace with appropriate hook call!
	
	((icall)iv->iv_Code)(0,0,iv->iv_Data, iv->iv_Code, SysBase); //on i386 it's just a plain call
	
	//	AROS_UFC5(void, iv->iv_Code,
	//	    AROS_UFCA(ULONG, 0, D1),
	//	    AROS_UFCA(ULONG, 0, A0),
	//	    AROS_UFCA(APTR, iv->iv_Data, A1),
	//	    AROS_UFCA(APTR, iv->iv_Code, A5),
	//	    AROS_UFCA(struct ExecBase *, SysBase, A6)
	//	);
	
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
		  
#warning direct call to aros, replace with appropriate hook call!
		  
#if NOISY
	fprintf(stderr,"********* sighandler: calling timer %p **********\n", iv->iv_Code);
	fflush(stderr);
#endif
		  ((icall)iv->iv_Code)(0,0,iv->iv_Data, iv->iv_Code, SysBase); //on i386 it's just a plain call
		  //		    AROS_UFC5(void, iv->iv_Code,
		  //			AROS_UFCA(ULONG, 0, D1),
		  //			AROS_UFCA(ULONG, 0, A0),
		  //			AROS_UFCA(APTR, iv->iv_Data, A1),
		  //			AROS_UFCA(APTR, iv->iv_Code, A5),
		  //			AROS_UFCA(struct ExecBase *, SysBase, A6)
		  //		    );
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
    if (SysBase->AttnResched & ARF_AttnDispatch)
    {
#if AROS_NESTING_SUPERVISOR
	  // Disable(); commented out, as causes problems with IDNestCnt. Getting completely out of range. 
#endif
	  UWORD u = (UWORD) ~(ARF_AttnDispatch);
	  AROS_ATOMIC_AND(SysBase->AttnResched, u);
	  
	  /* Save registers for this task (if there is one...) */
	  if (SysBase->ThisTask && SysBase->ThisTask->tc_State != TS_REMOVED)
	  {
	    SAVEREGS(SysBase->ThisTask, sc);
#if NOISY
	    printf("[Task switcher] saved context: ************\n");
            PRINT_CPUCONTEXT(SysBase->ThisTask);
#endif
	  }
	  
#if NOISY
	  fprintf(stderr,"********* sighandler: dispatching %p **********\n", Exec_Dispatch);
	  fflush(stderr);
#endif

	  /* Tell exec that we have actually switched tasks... */
	  //core_Dispatch ();
	  Exec_Dispatch(SysBase);
	  
#if NOISY
	  fprintf(stderr,"********* sighandler: dispatched %p **********\n", Exec_Dispatch);
	  fflush(stderr);
#endif

	  /* Get the registers of the old task */
	  RESTOREREGS(SysBase->ThisTask, sc);
#if NOISY
          fprintf(stderr,"********* sighandler: restored context  **********\n");
          PRINT_CPUCONTEXT(SysBase->ThisTask);
	  fflush(stderr);
#endif
	  
	  /* Make sure that the state of the interrupts is what the task
	   expects.
	   */
/* TODO
	  if (SysBase->IDNestCnt < 0)
	    SC_ENABLE(sc);
	  else
	    SC_DISABLE(sc);
*/	  
	  /* Ok, the next step is to either drop back to the new task, or
	   give it its Exception() if it wants one... */
	  
	  if (SysBase->ThisTask->tc_Flags & TF_EXCEPT)
	  {
#if NOISY
	fprintf(stderr,"********* sighandler: setting up exception %p **********\n",Exec_Exception);
	fflush(stderr);
#endif
		/* Exec_Exception will Disable() */
		do_enable();
		/* Make room for the current cpu context. */
		SysBase->ThisTask->tc_SPReg -= sizeof(CONTEXT);
		SP(GetCpuContext(SysBase->ThisTask)) = (DWORD)SysBase->ThisTask->tc_SPReg;
		/* Copy current cpu context. */
		memcpy(SysBase->ThisTask->tc_SPReg, GetCpuContext(SysBase->ThisTask), sizeof(CONTEXT));
		/* Manipulate the current cpu context so Exec_Exception gets
		 excecuted after we leave the kernel resp. the signal handler. */
		SP(GetCpuContext(SysBase->ThisTask)) = (unsigned long) SysBase->ThisTask->tc_SPReg;
		PC(GetCpuContext(SysBase->ThisTask)) = (unsigned long) Exec_Exception;
		SETUP_EXCEPTION(sc, SysBase);
		RESTOREREGS(SysBase->ThisTask, sc);
		
		if (SysBase->ThisTask->tc_SPReg <= SysBase->ThisTask->tc_SPLower)
		{
		  /* POW! */
		  fprintf(stderr, "aros just had what I beleive is a stack underrun");
		  while (1) {}
		  //   Alert(AT_DeadEnd|AN_StackProbe);
		}
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
  
  /* Are we returning from Exec_Exception? Then restore the saved cpu context. */
  if (SysBase->ThisTask && SysBase->ThisTask->tc_State == TS_EXCEPT)
  {
	SysBase->ThisTask->tc_SPReg = (APTR)SP(GetCpuContext(SysBase->ThisTask));
	memcpy(GetCpuContext(SysBase->ThisTask), SysBase->ThisTask->tc_SPReg, sizeof(CONTEXT));
	SysBase->ThisTask->tc_SPReg += sizeof(CONTEXT);
	
	if (SysBase->ThisTask->tc_SPReg > SysBase->ThisTask->tc_SPUpper)
	{
	  /* POW! */
	  fprintf(stderr, "aros just had what I beleive is a stack overrun");
	  while (1) {}
	  //            Alert(AT_DeadEnd|AN_StackProbe);
	}
	
	/* Restore the signaled context. */
	RESTOREREGS(SysBase->ThisTask,sc);
#if NOISY
        fprintf(stderr,"********* sighandler: restored context after exception **********\n");
        PRINT_CPUCONTEXT(SysBase->ThisTask);
	fflush(stderr);
#endif
	
	SysBase->ThisTask->tc_State = TS_RUN;
	do_enable();
  }
  
  /* Leave the interrupt. */
  AROS_ATOMIC_DEC(supervisor);
  
  sigactive[sig] = FALSE;
  
  printf("********* sighandler: done pc=%p *********\n",PC(GetCpuContext(SysBase->ThisTask)));

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

DWORD TaskSwitcher(HANDLE *ParentPtr)
{
    HANDLE IntEvent;
    DWORD obj;
    HANDLE MainThread = *ParentPtr;
    
    IntEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    for (;;) {
        obj = WaitForSingleObject(IntEvent, 1000000 / (50 * 2));
#ifdef NOISY
    	fprintf(stderr, "[Task switcher] signalled object %lu\n", obj);
	fflush(stderr);
#endif
    	SuspendThread(MainThread);
        DispatchEvent(obj, MainThread);
        ResumeThread(MainThread);
    }
    return 0;
}

/* Set up the kernel. */
void InitCore(void)
{
    HANDLE ThisProcess;
    HANDLE ThisThread, SwitcherThread;
    DWORD SwitcherId;
    
    ThisProcess = GetCurrentProcess();
    if (!DuplicateHandle(ThisProcess, GetCurrentThread(), ThisProcess, &ThisThread, 0, TRUE, DUPLICATE_SAME_ACCESS)) {
        Trace("[Scheduler] failed to get thread handle\n");
        return;
    }
    SwitcherThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TaskSwitcher, &ThisThread, 0, &SwitcherId);
    if (SwitcherThread) {
  	Trace("[Scheduler] started");
  	CloseHandle(SwitcherThread);
    } else
        Trace("[Scheduler] failed to run task switcher thread\n");

}

void * kernelSoftDisable (struct Hook * hook, unsigned long object, unsigned long message)
{
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  sigprocmask(SIG_BLOCK, &set, NULL);
  return 0;
}

void * kernelSoftEnable (struct Hook * hook, unsigned long object, unsigned long message)
{
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  sigprocmask(SIG_UNBLOCK, &set, NULL);
  return 0;
}

void * kernelIdleTask (struct Hook * hook, unsigned long object, unsigned long message)
{
  sigset_t sigs;
  sigemptyset(&sigs);
  while(1)
  {
	sigsuspend(&sigs);
  }
  return 0;
}

void * kernelSoftCause (struct Hook * hook, unsigned long object, unsigned long message)
{
  kill(getpid(), SIGUSR1);
  return 0;
}

void * kernelDisable (struct Hook * hook, unsigned long object, unsigned long message)
{
  do_disable();
  return 0;
}

void * kernelEnable (struct Hook * hook, unsigned long object, unsigned long message)
{
  do_enable();
  return 0;
}

void * kernelStartScheduler (struct Hook * hook, unsigned long object, unsigned long message)
{
  InitCore();
}

*/
