/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.11  1996/09/17 18:41:18  digulla
    This file now contains a DOSBase for internal use in the OS only (and
    	if you can, don't use it but open the dos.library on your own). I'll try
    	to remove again, later.

    Revision 1.10  1996/09/17 16:17:02	digulla
    Moved CreateNewProc() in front of start of timer, because of crashes if
	the timer is enabled. But that's not enough yet :(

    Revision 1.9  1996/09/13 17:57:08  digulla
    Use IPTR

    Revision 1.8  1996/08/30 17:00:59  digulla
    Tried a timer with higher resolution to have 50 task switches per second,
    but it crashes. If someone wants to debug it, define ENABLE_TIMER and compile
    init.c anew.

    Revision 1.7  1996/08/28 17:57:37  digulla
    Doesn't call intui_ProcessXEvents() andmore but signals the input.device.
    This will change in the future but as long as we don't have real multitasking,
    there is no other way to do it.

    Revision 1.6  1996/08/23 17:12:28  digulla
    Added several new aros specific includes
    We have now a console.device
    The memory is allocated now and not part of the BSS so illegal accesses show
	up earlier now.
    New global variable: AROSBase. Can be accesses from anywhere via
	SysBase->DebugData for now. Will be used for RT and Purify.
    AROSBase.StdOut is a FILE*-handle for use in kprintf() but that doesn't
	seem to work in all cases

    Revision 1.5  1996/08/15 13:21:06  digulla
    A couple of comments

    Revision 1.4  1996/08/13 14:04:33  digulla
    Added intui_ProcessXEvents() to Idle-Task
    Added graphics and intuition.library to system libraries
    Renamed stdin, stdout and stderr to allow to use stdio.h

    Revision 1.3  1996/08/03 20:20:55  digulla
    Tried to add multitasking but that doesn't work right now

    Revision 1.2  1996/08/01 17:41:25  digulla
    Added standard header for all files

    Desc: startup code for AROS (main())
    Lang: english
*/
#include <stdlib.h>
#include <signal.h>
#define timeval     linux_timeval
#include <sys/time.h>
#undef timeval
#include <unistd.h>
#include <stdio.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/devices.h>
#include <clib/aros_protos.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <clib/dos_protos.h>
#include <utility/tagitem.h>
#include <aros/rt.h>
#include <aros/arosbase.h>
#include "memory.h"
#include "machine.h"
#undef kprintf

#define NEWLIST(l)                          \
((l)->lh_Head=(struct Node *)&(l)->lh_Tail, \
 (l)->lh_Tail=NULL,                         \
 (l)->lh_TailPred=(struct Node *)(l))

extern void *ExecFunctions[];
extern const struct Resident Utility_resident;
extern const struct Resident Dos_resident;
extern const struct Resident Graphics_resident;
extern const struct Resident Intuition_resident;
extern const struct Resident emul_handler_resident;
extern const struct Resident Console_resident;

#define MEMSIZE 1024*1024
static struct MemHeader mh;
/* static UBYTE memory[MEMSIZE+MEMCHUNK_TOTAL]; */
UBYTE * memory;

#define NUMVECT 131

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;

#define STACKSIZE 4096

static int returncode=20;
static struct AROSBase AROSBase;

extern struct Task * inputDevice;

#ifndef ENABLE_TIMER
#   define ENABLE_TIMER     0
#endif

static void idleTask (void)
{
    /* If the idle task ever gets CPU time the emulation is finished */
/* exit(returncode); */
    while (1)
    {
#if !ENABLE_TIMER
	Signal (inputDevice, SIGBREAKF_CTRL_F);
#endif

	Switch (); /* Rescedule */
    }
}

int submain(int argc,char *argv[]);

static int gargc;
static char **gargv;

static void boot(void)
{
    returncode=submain(gargc,gargv);
    RemTask(NULL);
}

#if ENABLE_TIMER
static void timer (int dummy)
{
    /* Let input.device live */
    Signal (inputDevice, SIGBREAKF_CTRL_F);

    signal (SIGALRM, timer);
}
#endif

static APTR allocmem(ULONG size)
{
    UBYTE *ret;

    size=(size+MEMCHUNK_TOTAL-1)&~(MEMCHUNK_TOTAL-1);
    ret=(UBYTE *)mh.mh_First;
    mh.mh_First=(struct MemChunk *)(ret+size);
    mh.mh_First->mc_Next=NULL;
    mh.mh_Free=mh.mh_First->mc_Bytes=((struct MemChunk *)ret)->mc_Bytes-size;
    return ret;
}

int main(int argc,char *argv[])
{
    ULONG * space;

    /* Put arguments into globals */
    gargc=argc;
    gargv=argv;

    /* Leave a space of 4096 bytes before the memory */
    space = malloc (4096);
    memory = malloc (MEMSIZE+MEMCHUNK_TOTAL);

    { /* erase space */
	int size = 4096/sizeof(ULONG);

	while (--size)
	    *space ++ = 0xDEADBEEF;
    }

    /*
	Prepare first MemHeader. I cannot use exec functions
	here because exec is not yet up.
    */
    mh.mh_Node.ln_Name="unknown memory type";
    mh.mh_Node.ln_Pri =0;
    mh.mh_Attributes  =MEMF_FAST|MEMF_PUBLIC; /* Public to my emulation */
    mh.mh_First=(struct MemChunk *)
		(((IPTR)memory+MEMCHUNK_TOTAL-1)&~(MEMCHUNK_TOTAL-1));
    mh.mh_First->mc_Next=NULL;
    mh.mh_First->mc_Bytes=MEMSIZE;
    mh.mh_Lower=mh.mh_First;
    mh.mh_Upper=(UBYTE *)mh.mh_Lower+MEMSIZE;
    mh.mh_Free =MEMSIZE;

    /* The following allocations cannot and must not fail. */
    {
	/* Prepare exec.library */
	ULONG neg,i;
	neg=sizeof(struct JumpVec)*NUMVECT;
	neg=(neg+LIBALIGN-1)&~(LIBALIGN-1);
	SysBase=(struct ExecBase *)
		((UBYTE *)allocmem(neg+sizeof(struct ExecBase))+neg);
	for(i=0;i<NUMVECT;i++)
	{
	    SET_JMP(&((struct JumpVec *)SysBase)[-i-1]);
	    SET_VEC(&((struct JumpVec *)SysBase)[-i-1],ExecFunctions[i]);
	}
#if 0
	/* Build GetCC vector (68000 version) */
	((UWORD *)((UBYTE *)SysBase-88*LIB_VECTSIZE))[0]=0x40c0; /* movew sr,d0 */
	((UWORD *)((UBYTE *)SysBase-88*LIB_VECTSIZE))[1]=0x4e75; /* rts         */
#endif

	SysBase->LibNode.lib_Node.ln_Name="exec.library";
	SysBase->DebugData = &AROSBase;

	AROSBase.kprintf = (void *)kprintf;

	NEWLIST(&SysBase->MemList);
	AddHead(&SysBase->MemList,&mh.mh_Node);
	NEWLIST(&SysBase->ResourceList);
	NEWLIST(&SysBase->DeviceList);
	NEWLIST(&SysBase->IntrList);
	NEWLIST(&SysBase->LibList);
	NEWLIST(&SysBase->PortList);
	NEWLIST(&SysBase->TaskReady);
	NEWLIST(&SysBase->TaskWait);

	for(i=0;i<5;i++)
	{
	    NEWLIST(&SysBase->SoftInts[i].sh_List);
	}

	NEWLIST(&SysBase->SemaphoreList);

	/* There are no memhandlers yet.
	 * (not even the library flushing one which is part of ram/dos.library) */
	NEWLIST((struct List *)&SysBase->ex_MemHandlers);
	SysBase->IDNestCnt=0;
	SysBase->TDNestCnt=0;
	SysBase->AttnResched=0;
    }

    {
	/* Add boot task */
	struct Task *t;
	struct MemList *ml;

	ml=(struct MemList *)AllocMem(sizeof(struct MemList),MEMF_PUBLIC|MEMF_CLEAR);
	t =(struct Task *)   AllocMem(sizeof(struct Task),   MEMF_PUBLIC|MEMF_CLEAR);
	ml->ml_NumEntries     =1;
	ml->ml_ME[0].me_Addr  =t;
	ml->ml_ME[0].me_Length=sizeof(struct Task);

	NEWLIST(&t->tc_MemEntry);
	AddHead(&t->tc_MemEntry,&ml->ml_Node);
	t->tc_Node.ln_Name="Boot task";
	t->tc_Node.ln_Pri=0;
	t->tc_State=TS_RUN;
	t->tc_SigAlloc=0xffff;
	SysBase->ThisTask=t;
    }
    {
	/* Add idle task */
	struct Task *t;
	struct MemList *ml;
	UBYTE *s;

	/* Allocate one header (incl. the first entry) and one additional
	    entry */
	ml=(struct MemList *)AllocMem(sizeof(struct MemList)+sizeof(struct MemEntry),
				      MEMF_PUBLIC|MEMF_CLEAR);
	t =(struct Task *)   AllocMem(sizeof(struct Task),    MEMF_PUBLIC|MEMF_CLEAR);
	s =(UBYTE *)         AllocMem(STACKSIZE,              MEMF_PUBLIC|MEMF_CLEAR);
	ml->ml_NumEntries     =2;
	ml->ml_ME[0].me_Addr  =t;
	ml->ml_ME[0].me_Length=sizeof(struct Task);
	ml->ml_ME[1].me_Addr  =s;
	ml->ml_ME[1].me_Length=STACKSIZE;

	NEWLIST(&t->tc_MemEntry);
	AddHead(&t->tc_MemEntry,&ml->ml_Node);
	t->tc_SPLower=s;
	t->tc_SPUpper=s+STACKSIZE;
	t->tc_Node.ln_Name="Idle task";
	t->tc_Node.ln_Pri=-128;
#if STACK_GROWS_DOWNWARDS
	t->tc_SPReg=(UBYTE *)t->tc_SPUpper-SP_OFFSET;
#else
	t->tc_SPReg=(UBYTE *)t->tc_SPLower-SP_OFFSET;
#endif
	AddTask(t,&idleTask,NULL);
    }
    Enable();
    Permit();

    AddLibrary((struct Library *)InitResident((struct Resident *)&Utility_resident,0));

    DOSBase = (struct DosLibrary *)InitResident((struct Resident *)&Dos_resident,0);

    AddLibrary((struct Library *)DOSBase);

    AddLibrary((struct Library *)InitResident((struct Resident *)&Graphics_resident,0));
    AddLibrary((struct Library *)InitResident((struct Resident *)&Intuition_resident,0));

    {
	struct consolebase
	{
	    struct Device device;
	};

	struct consolebase *conbase;

	conbase=(struct consolebase *)InitResident((struct Resident *)&Console_resident,0);
	AddDevice (&conbase->device);
    }

    DOSBase = (struct DosLibrary *)OpenLibrary (DOSNAME, 39);

    if (!DOSBase)
    {
	fprintf (stderr, "Cannot open dos.library");
	exit (10);
    }

    {
	struct emulbase
	{
	    struct Device eb_device;
	    struct Unit *eb_stdin;
	    struct Unit *eb_stdout;
	    struct Unit *eb_stderr;
	};

	struct emulbase *emulbase;

	struct TagItem fhtags[]=
	{ { TAG_END, 0 } };

	struct FileHandle *fh_stdin=(struct FileHandle *)AllocDosObject(DOS_FILEHANDLE,fhtags);
	struct FileHandle *fh_stdout=(struct FileHandle *)AllocDosObject(DOS_FILEHANDLE,fhtags);

	struct TagItem bootprocess[]=
	{
	    { NP_Entry, (IPTR)boot },
	    { NP_Input, MKBADDR(fh_stdin) },
	    { NP_Output, MKBADDR(fh_stdout) },
	    { NP_Name, (IPTR)"Boot process" },
	    { NP_StackSize, 8000 }, /* linux's printf needs that much. */
	    { NP_Cli, 1 },
	    { TAG_END, 0 }
	};

	emulbase=(struct emulbase *)InitResident((struct Resident *)&emul_handler_resident,0);
	AddDevice(&emulbase->eb_device);

	AssignLock("C",Lock("SYS:c",SHARED_LOCK));
	AssignLock("S",Lock("SYS:s",SHARED_LOCK));
	AssignLock("Libs",Lock("SYS:libs",SHARED_LOCK));
	AssignLock("Devs",Lock("SYS:devs",SHARED_LOCK));

	fh_stdin->fh_Device=&emulbase->eb_device;
	fh_stdin->fh_Unit  =emulbase->eb_stdin;
	fh_stdout->fh_Device=&emulbase->eb_device;
	fh_stdout->fh_Unit  =emulbase->eb_stdout;

	/* AROSBase.StdOut = MKBADDR(fh_stdout); */
	AROSBase.StdOut = stderr;

	CreateNewProc(bootprocess);

#if ENABLE_TIMER
	{
	    struct itimerval interval;
	    int rc;

	    /* Start Multitasking (not yet) */
	    signal (SIGALRM, timer);

	    interval.it_interval.tv_sec = interval.it_value.tv_sec = 0;
	    interval.it_interval.tv_usec = interval.it_value.tv_usec = 1000000/50;

	    rc = setitimer (ITIMER_REAL, &interval, NULL);
	}
#endif
    }

    RemTask(NULL); /* get rid of Boot task */

    /* Get compiler happy */
    return 0;
}
