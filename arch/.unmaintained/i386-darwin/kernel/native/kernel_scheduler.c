/*
 Copyright © 1995-2007, The AROS Development Team. All rights reserved.
 $Id$
 
 Desc: Initialize the interface to the "hardware".
 Lang: english
 */

#define timeval sys_timeval
#include "sigcore.h"
#include "../include/aros/kernel.h"
#include <libkern/OSAtomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#undef timeval


#define GetIntETask(task)   ((struct IntETask *)(((struct Task *) \
				(task))->tc_UnionETask.tc_ETask))
#define IntETask(etask)	    ((struct IntETask *)(etask))


#define INTB_TBE          0
#define INTF_TBE     (1L<<0)
#define INTB_DSKBLK       1
#define INTF_DSKBLK  (1L<<1)
#define INTB_SOFTINT      2
#define INTF_SOFTINT (1L<<2)
#define INTB_PORTS        3
#define INTF_PORTS   (1L<<3)
#define INTB_COPER        4
#define INTF_COPER   (1L<<4)
#define INTB_VERTB        5
#define INTF_VERTB   (1L<<5)
#define INTB_BLIT         6
#define INTF_BLIT    (1L<<6)
#define INTB_AUD0         7
#define INTF_AUD0    (1L<<7)
#define INTB_AUD1         8
#define INTF_AUD1    (1L<<8)
#define INTB_AUD2         9
#define INTF_AUD2    (1L<<9)
#define INTB_AUD3         10
#define INTF_AUD3    (1L<<10)
#define INTB_RBF          11
#define INTF_RBF     (1L<<11)
#define INTB_DSKSYNC      12
#define INTF_DSKSYNC (1L<<12)
#define INTB_EXTER        13
#define INTF_EXTER   (1L<<13)
#define INTB_INTEN        14
#define INTF_INTEN   (1L<<14)
#define INTB_SETCLR       15
#define INTF_SETCLR  (1L<<15)

struct Interrupt
{
    struct Node is_Node;
    APTR        is_Data;
    VOID     (* is_Code)(); /* server code entry */
};

/* PRIVATE */
struct IntVector
{
    APTR          iv_Data;
    VOID       (* iv_Code)();
    struct Node * iv_Node;
};

/* PRIVATE */
struct SoftIntList
{
    struct List sh_List;
    UWORD       sh_Pad;
};

#define SIH_PRIMASK (0xf0)

#define INTB_NMI      15
#define INTF_NMI (1L<<15)

/* Library structure. Also used by Devices and some Resources. */
struct Library {
    struct  Node lib_Node;
    UBYTE   lib_Flags;
    UBYTE   lib_pad;
    UWORD   lib_NegSize;	    /* number of bytes before library */
    UWORD   lib_PosSize;	    /* number of bytes after library */
    UWORD   lib_Version;	    /* major */
    UWORD   lib_Revision;	    /* minor */
#ifdef AROS_NEED_LONG_ALIGN
    UWORD   lib_pad1;		    /* make sure it is longword aligned */
#endif
    APTR    lib_IdString;	    /* ASCII identification */
    ULONG   lib_Sum;		    /* the checksum */
    UWORD   lib_OpenCnt;	    /* How many people use us right now ? */
#ifdef AROS_NEED_LONG_ALIGN
    UWORD   lib_pad2;		    /* make sure it is longword aligned */
#endif
};


/* Most fields are PRIVATE */
struct ExecBase
{
/* Standard Library Structure */
    struct Library LibNode;

/* System Constants */
    UWORD SoftVer;      /* OBSOLETE */
    WORD  LowMemChkSum;
    IPTR  ChkBase;
    APTR  ColdCapture;
    APTR  CoolCapture;
    APTR  WarmCapture;
    APTR  SysStkUpper;  /* System Stack Bounds */
    APTR  SysStkLower;
    IPTR  MaxLocMem;    /* Chip Memory Pointer */
    APTR  DebugEntry;
    APTR  DebugData;
    APTR  AlertData;
    APTR  MaxExtMem;    /* Extended Memory Pointer (may be NULL) */
    UWORD ChkSum;       /* SoftVer to MaxExtMem */

/* Interrupts */
    struct IntVector IntVects[16];

/* System Variables */
    struct Task * ThisTask;       /* Pointer to currently running task
                                     (readable) */
    ULONG        IdleCount;
    ULONG        DispCount;
    UWORD        Quantum;        /* # of ticks, a task may run */
    UWORD        Elapsed;        /* # of ticks, the current task has run */
    UWORD        SysFlags;
    BYTE         IDNestCnt;
    BYTE         TDNestCnt;
    UWORD        AttnFlags;      /* Attention Flags (see below) (readable) */
    UWORD        AttnResched;
    APTR         ResModules;
    APTR         TaskTrapCode;
    APTR         TaskExceptCode;
    APTR         TaskExitCode;
    ULONG        TaskSigAlloc;
    UWORD        TaskTrapAlloc;

/* PRIVATE Lists */
    struct List        MemList;
    struct List        ResourceList;
    struct List        DeviceList;
    struct List        IntrList;
    struct List        LibList;
    struct List        PortList;
    struct List        TaskReady;      /* Tasks that are ready to run */
    struct List        TaskWait;       /* Tasks that wait for some event */
    struct SoftIntList SoftInts[5];

/* Miscellaneous Stuff */
    LONG               LastAlert[4];

    UBYTE              VBlankFrequency;      /* (readable) */
    UBYTE              PowerSupplyFrequency; /* (readable) */
    	    	    	    	    	     /* AROS PRIVATE: VBlankFreq * PowerSupplyFreq = Timer Tick Rate */
    struct List        SemaphoreList;

/* Kickstart */
    APTR KickMemPtr;
    APTR KickTagPtr;
    APTR KickCheckSum;

/* Miscellaneous Stuff */
    UWORD          ex_Pad0;            /* PRIVATE */
    IPTR           ex_LaunchPoint;     /* PRIVATE */
    APTR           ex_RamLibPrivate;
    ULONG          ex_EClockFrequency; /* (readable) */
    ULONG          ex_CacheControl;    /* PRIVATE */
    ULONG          ex_TaskID;
    ULONG          ex_Reserved1[5];
    APTR           ex_MMULock;         /* PRIVATE */
    ULONG          ex_Reserved2[3];
    struct MinList ex_MemHandlers;
    APTR           ex_MemHandler;      /* PRIVATE */

/* Additional fields for AROS */
    struct Library      * DebugAROSBase;
    void                * PlatformData;     /* different for all platforms */
};

/* AttnFlags */
/* Processors */
#define AFB_68010        0
#define AFF_68010   (1L<<0)
#define AFB_68020        1
#define AFF_68020   (1L<<1)
#define AFB_68030        2
#define AFF_68030   (1L<<2)
#define AFB_68040        3
#define AFF_68040   (1L<<3)
/* Co-Processors */
#define AFB_68881        4
#define AFF_68881   (1L<<4)
#define AFB_68882        5
#define AFF_68882   (1L<<5)
#define AFB_FPU40        6
#define AFF_FPU40   (1L<<6)
#define AFB_PRIVATE      15 /* PRIVATE */
#define AFF_PRIVATE (1L<<15)

/* Cache */
#define CACRF_EnableI       (1L<<0)
#define CACRF_FreezeI       (1L<<1)
#define CACRF_ClearI        (1L<<3)
#define CACRF_IBE           (1L<<4)
#define CACRF_EnableD       (1L<<8)
#define CACRF_FreezeD       (1L<<9)
#define CACRF_ClearD        (1L<<11)
#define CACRF_DBE           (1L<<12)
#define CACRF_WriteAllocate (1L<<13)
#define CACRF_InvalidateD   (1L<<15)
#define CACRF_EnableE       (1L<<30)
#define CACRF_CopyBack      (1L<<31)

/* DMA */
#define DMA_Continue    (1L<<1)
#define DMA_NoModify    (1L<<2)
#define DMA_ReadFromRAM (1L<<3)

/* AROS extensions */

/* SysBase->VBlankFrequency * SysBase->PowerSupplyFrequency Hz timer */
#define INTB_TIMERTICK 	INTB_COPER
#define INTF_TIMERTICK	INTF_COPER


#define ARF_AttnDispatch    (1L<<15)

#define DEBUG_TT    0
#if DEBUG_TT
struct Task * lastTask;
#endif

#define TASKTAG_Dummy	(TAG_USER + 0x100000)
#define TASKTAG_ARG1	(TASKTAG_Dummy + 16)
#define TASKTAG_ARG2	(TASKTAG_Dummy + 17)
#define TASKTAG_ARG3	(TASKTAG_Dummy + 18)
#define TASKTAG_ARG4	(TASKTAG_Dummy + 19)
#define TASKTAG_ARG5	(TASKTAG_Dummy + 20)
#define TASKTAG_ARG6	(TASKTAG_Dummy + 21)
#define TASKTAG_ARG7	(TASKTAG_Dummy + 22)
#define TASKTAG_ARG8	(TASKTAG_Dummy + 23)

/* tc_Flags Bits */
#define TB_PROCTIME	0
#define TB_ETASK	3
#define TB_STACKCHK	4
#define TB_EXCEPT	5
#define TB_SWITCH	6
#define TB_LAUNCH	7

#define TF_PROCTIME	(1L<<0)
#define TF_ETASK	(1L<<3)
#define TF_STACKCHK	(1L<<4)
#define TF_EXCEPT	(1L<<5)
#define TF_SWITCH	(1L<<6)
#define TF_LAUNCH	(1L<<7)

/* Task States (tc_State) */
#define TS_INVALID	0
#define TS_ADDED	1
#define TS_RUN		2
#define TS_READY	3
#define TS_WAIT		4
#define TS_EXCEPT	5
#define TS_REMOVED	6

/* Predefined Signals */
#define SIGB_ABORT	0
#define SIGB_CHILD	1
#define SIGB_BLIT	4	/* Note: same as SIGB_SINGLE */
#define SIGB_SINGLE	4	/* Note: same as SIGB_BLIT */
#define SIGB_INTUITION	5
#define SIGB_NET	7
#define SIGB_DOS	8

#define SIGF_ABORT	(1L<<0)
#define SIGF_CHILD	(1L<<1)
#define SIGF_BLIT	(1L<<4)
#define SIGF_SINGLE	(1L<<4)
#define SIGF_INTUITION	(1L<<5)
#define SIGF_NET	(1L<<7)
#define SIGF_DOS	(1L<<8)

/* Don't do any debugging. At 50Hz its far too quick to read anyway :-) */
#define NOISY	1

#define Trace(s) {printf(s);printf("\n");fflush(stdout);}

/* Try and emulate the Amiga hardware interrupts */
int sig2tab[NSIG];
sigset_t sig_int_mask;	/* Mask of signals that Disable() blocks */
int intrap;
int supervisor;
extern struct ExecBase * SysBase;
int sigactive[NSIG];

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

/* On TF_EXCEPT make Exec_Exception being called. */
extern void (*Exec_Exception)(struct ExecBase*);
extern void (*Exec_Dispatch)(struct ExecBase*);

/* This is from sigcore.h - it brings in the definition of the
 systems initial signal handler, which simply calls
 sighandler(int signum, sigcontext_t sigcontext)
 */
GLOBAL_SIGNAL_INIT

void do_enable()
{

  OSAtomicDecrement32(&SysBase->IDNestCnt);
  
  if(SysBase->IDNestCnt < 0)
  {
	sigprocmask(SIG_UNBLOCK, &sig_int_mask, NULL);
  }
}

void do_disable()
{

  sigprocmask(SIG_BLOCK, &sig_int_mask, NULL);
  
  OSAtomicIncrement32(&SysBase->IDNestCnt);
  
  if (SysBase->IDNestCnt < 0)
  {
	/* If we get here we have big trouble. Someone called
	 1x Disable() and 2x Enable(). IDNestCnt < 0 would
	 mean enable interrupts, but the caller of Disable
	 relies on the function to disable them, so we donÂ¥t
	 do anything here (or maybe a deadend alert?) */
   printf("negative nest count!\n");
  }
}


/* sighandler() Handle the signals:
 You can either turn them into interrupts, or alternatively,
 you can turn them into traps (eg software failures)
 */
static void sighandler(int sig, sigcontext_t * sc)
{
#if NOISY
	printf("************* sighandler signal : 0x%x context: ************\n",sig);
  PRINT_SC(sc);
#endif

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
  
  OSAtomicIncrement32 (&supervisor);
  //    AROS_ATOMIC_INC(supervisor);
  
  if (sigactive[sig])
  {
#if NOISY
	fprintf(stderr,"********* sighandler: sig %d already active **********\n", sig);
	fflush(stderr);
#endif
	
	return;
  }
  sigactive[sig] = 1;
  
  /* Map the Unix signal to an Amiga signal. */
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
	
	
	OSAtomicIncrement32 (&SysBase->IDNestCnt);
	//	AROS_ATOMIC_INC(SysBase->IDNestCnt);
	
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
	
	OSAtomicDecrement32 (&SysBase->IDNestCnt);
	//	AROS_ATOMIC_DEC(SysBase->IDNestCnt);
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
	  OSAtomicAnd32 (u, &SysBase->AttnResched);
	  //        AROS_ATOMIC_AND(SysBase->AttnResched, u);
	  
	  /* Save registers for this task (if there is one...) */
	  if (SysBase->ThisTask && SysBase->ThisTask->tc_State != TS_REMOVED)
	  {
	    SAVEREGS(SysBase->ThisTask, sc);
#if NOISY
	fprintf(stderr,"********* sighandler: saved task context: **********\n", Exec_Dispatch);
	fflush(stderr);
  PRINT_SC(sc);
#endif
	  }
	  
#if NOISY
	fprintf(stderr,"********* sighandler: dispatching %p **********\n", Exec_Dispatch);
	fflush(stderr);
#endif

	  /* Tell exec that we have actually switched tasks... */
	  //core_Dispatch ();
	  Exec_Dispatch(SysBase);

	  /* Get the registers of the old task */
	  RESTOREREGS(SysBase->ThisTask, sc);
	  
#if NOISY
    fprintf(stderr,"********* sighandler: restored context  **********\n");
    PRINT_SC(sc);
	fflush(stderr);
#endif
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
#if NOISY
	fprintf(stderr,"********* sighandler: setting up exception %p **********\n",Exec_Exception);
	fflush(stderr);
#endif
		/* Exec_Exception will Disable() */
		do_enable();
		/* Make room for the current cpu context. */
		SysBase->ThisTask->tc_SPReg -= SIZEOF_ALL_REGISTERS;
		GetCpuContext(SysBase->ThisTask)->sc = SysBase->ThisTask->tc_SPReg;
		/* Copy current cpu context. */
		memcpy(SysBase->ThisTask->tc_SPReg, GetCpuContext(SysBase->ThisTask), SIZEOF_ALL_REGISTERS);
		/* Manipulate the current cpu context so Exec_Exception gets
		 excecuted after we leave the kernel resp. the signal handler. */
		SP(sc) = (unsigned long) SysBase->ThisTask->tc_SPReg;
		PC(sc) = (unsigned long) Exec_Exception;
		SETUP_EXCEPTION(sc, SysBase);
		
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
	SysBase->ThisTask->tc_SPReg = GetCpuContext(SysBase->ThisTask)->sc;
	memcpy(GetCpuContext(SysBase->ThisTask), SysBase->ThisTask->tc_SPReg, SIZEOF_ALL_REGISTERS);
	SysBase->ThisTask->tc_SPReg += SIZEOF_ALL_REGISTERS;
	
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
    PRINT_SC(sc);
	fflush(stderr);
#endif
	
	SysBase->ThisTask->tc_State = TS_RUN;
	do_enable();
  }
  
  /* Leave the interrupt. */
  
  OSAtomicDecrement32 (&supervisor);
  //    AROS_ATOMIC_DEC(supervisor);
  
  sigactive[sig] = FALSE;
  
	printf("********* sighandler: done pc=%p *********\n",PC(sc));

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
  
  Trace("[Scheduler] setting up signal flags");
  
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
  sa.sa_sigaction = (void *)SIGHANDLER;
  sa.sa_flags |= SA_SIGINFO;

  sa.sa_mask = sig_int_mask;
  for(i=0; i < (sizeof(sig2int) / sizeof(sig2int[0])); i++)
  {
	sig2tab[sig2int[i][0]] = sig2int[i][1];
	sigaction( sig2int[i][0], &sa, NULL );
  }
  sigaction( SIGINT, &sa, NULL );
  
  Trace("[Scheduler] starting vblank timer");

  /* Set up the "pseudo" vertical blank interrupt. */
  interval.it_interval.tv_sec = interval.it_value.tv_sec = 0;
  interval.it_interval.tv_usec =
  interval.it_value.tv_usec = 1000000 / (SysBase->VBlankFrequency * SysBase->PowerSupplyFrequency);
  interval.it_value.tv_usec = 1000000 / (50 * 2);

//  setitimer(ITIMER_REAL, &interval, NULL);

  Trace("[Scheduler] started");

}

void do_prepare_context(void * ptask, void * entryPoint, void * fallBack, struct TagItem * tagList)
{
  struct Task * task = (struct Task*)ptask;
  IPTR args[8] = {0};
  WORD numargs = 0;
  while(tagList)
  {
	switch(tagList->ti_Tag)
	{
	  case TAG_MORE:
		tagList = (struct TagItem *)tagList->ti_Data;
		continue;
		
	  case TAG_SKIP:
		tagList += tagList->ti_Data;
		break;
		
	  case TAG_DONE:
		tagList = NULL;
		break;
		
#define HANDLEARG(x) \
case TASKTAG_ARG ## x: \
args[x - 1] = (IPTR)tagList->ti_Data; \
if (x > numargs) numargs = x; \
break;
		
		HANDLEARG(1)
		HANDLEARG(2)
		HANDLEARG(3)
		HANDLEARG(4)
		HANDLEARG(5)
		HANDLEARG(6)
		HANDLEARG(7)
		HANDLEARG(8)
		
#undef HANDLEARG
	}
	
	if (tagList) tagList++;
  }
  if (numargs)
  {

	/* Assume C function gets all param on stack */
	
	while(numargs--)
	{
    printf("  arg %i: %p\n",args[numargs]);
	  _PUSH(GetSP(task), args[numargs]);
	}
	
  }
  
  /* First we push the return address */
  _PUSH(GetSP(task), fallBack);
  
  /* Then set up the frame to be used by Dispatch() */
  PREPARE_INITIAL_FRAME(GetSP(task), entryPoint);
  PREPARE_INITIAL_CONTEXT(task, entryPoint);

  printf("prepared task, context:\n");
  PRINT_CPUCONTEXT(task);

}

void * kernelPrepareContext(struct Hook * hook, unsigned long object, unsigned long message)
{
  struct TagItem * msg = (struct TagItem *)message;
  void * entryPoint = (void *)(msg->ti_Data); ++msg;
  void * fallback = (void *)(msg->ti_Data); ++msg;  
  struct TagItem * taglist = (struct TagItem *)(msg->ti_Data);
  do_prepare_context((void *)object, entryPoint, fallback, taglist);
  return 0;
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

void * kernelPutchar (struct Hook * hook, unsigned long object, unsigned long c)
{
  putchar(c);
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

