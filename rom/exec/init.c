/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/08/13 14:04:33  digulla
    Added intui_ProcessXEvents() to Idle-Task
    Added graphics and intuition.library to system libraries
    Renamed stdin, stdout and stderr to allow to use stdio.h

    Revision 1.3  1996/08/03 20:20:55  digulla
    Tried to add multitasking but that doesn't work right now

    Revision 1.2  1996/08/01 17:41:25  digulla
    Added standard header for all files

    Desc:
    Lang:
*/

#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/devices.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <clib/dos_protos.h>
#include <utility/tagitem.h>
#include "memory.h"
#include "machine.h"
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

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

#define MEMSIZE 1024*1024
static struct MemHeader mh;
static UBYTE memory[MEMSIZE+MEMCHUNK_TOTAL-1];

#define NUMVECT 131

struct ExecBase *SysBase;
struct DosBase * DOSBase;

#define STACKSIZE 4096

static int returncode=20;

void intui_ProcessXEvents (void);

static void idleTask (void)
{
    /* If the idle task ever gets CPU time the emulation is finished */
/* exit(returncode); */
    while (1)
    {
	intui_ProcessXEvents ();

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

static void timer (int dummy)
{
    signal (SIGALRM, timer);
    alarm (1);
    Switch ();
}

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
    /* Put arguments into globals */
    gargc=argc;
    gargv=argv;

    /*
	Prepare first MemHeader. I cannot use exec functions
	here because exec is not yet up.
    */
    mh.mh_Node.ln_Name="unknown memory type";
    mh.mh_Node.ln_Pri =0;
    mh.mh_Attributes  =MEMF_FAST|MEMF_PUBLIC; /* Public to my emulation */
    mh.mh_First=(struct MemChunk *)
		(((ULONG)memory+MEMCHUNK_TOTAL-1)&~(MEMCHUNK_TOTAL-1));
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
    AddLibrary((struct Library *)InitResident((struct Resident *)&Dos_resident,0));
    AddLibrary((struct Library *)InitResident((struct Resident *)&Graphics_resident,0));
    AddLibrary((struct Library *)InitResident((struct Resident *)&Intuition_resident,0));

    DOSBase = (struct DosBase *) OpenLibrary (DOSNAME, 39);

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
	    { NP_Entry, (ULONG)boot },
	    { NP_Input, MKBADDR(fh_stdin) },
	    { NP_Output, MKBADDR(fh_stdout) },
	    { NP_Name, (ULONG)"Boot process" },
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

	/* Start Multitasking (not yet) * /
	signal (SIGALRM, timer);
	alarm (1); */

	CreateNewProc(bootprocess);
    }
    RemTask(NULL);

    /* Get compiler happy */
    return 0;
}
