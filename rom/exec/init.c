/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: startup code for AROS (main())
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <aros/system.h>
#include <stdlib.h>
#include <signal.h>
#ifndef _AMIGA
#define timeval     linux_timeval
#include <sys/time.h>
#undef timeval
#include <sys/mman.h>
#else
#include <sys/time.h>
#endif
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/devices.h>
#include <proto/arossupport.h>
#include <proto/exec.h>
#include "exec_pdefs.h"
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <utility/tagitem.h>
#include <aros/rt.h>
#include <aros/arossupportbase.h>
#include <hardware/intbits.h>
#include <hardware/custom.h>
#include "memory.h"
#include <aros/machine.h>
#include <aros/asmcall.h>
#undef kprintf

extern void *ExecFunctions[];
extern const struct Resident Utility_resident;
extern const struct Resident Dos_resident;
extern const struct Resident Graphics_resident;
extern const struct Resident Intuition_resident;
extern const struct Resident emul_handler_resident;
extern const struct Resident Console_resident;
extern const struct Resident Mathffp_resident;

extern void InitCore(void);

#define MEMSIZE 1024*1024*2
/*#define STATIC_MEMORY */ /* So that gdb can disassemble it */
static struct MemHeader mh;
#ifdef STATIC_MEMORY
static UBYTE memory[MEMSIZE+MEMCHUNK_TOTAL];
#else
UBYTE * memory;
#endif

#define NUMVECT 131

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;

static int returncode=20;
static struct AROSSupportBase AROSSupportBase;

extern struct Task * inputDevice;
extern void debugmem (void);

static void idleTask (void)
{
    while (1)
    {
	if (inputDevice)
	    Signal (inputDevice, SIGBREAKF_CTRL_F);
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

AROS_UFH5 (void, IntServer,
    AROS_UFHA (ULONG             ,dummy,  D0),
    AROS_UFHA (struct Custom    *,custom, A0),
    AROS_UFHA (struct List      *,intList,A1),
    AROS_UFHA (APTR              ,ivCode, A5),
    AROS_UFHA (struct ExecBase  *,SysBase,A6)
)
{
    struct Interrupt * irq;

    ForeachNode (intList, irq)
    {
	if( AROS_UFC4( int, irq->is_Code,
		AROS_UFCA (struct Custom *,   custom,       A0),
		AROS_UFCA (APTR,              irq->is_Data, A1),
		AROS_UFCA (APTR,              irq->is_Code, A5),
		AROS_UFCA (struct ExecBase *, SysBase,      A6)
	))
	    break;
    }
}

AROS_UFH4(int, Dispatcher,
    AROS_UFHA (struct Custom   *, custom,  A0),
    AROS_UFHA (APTR,              is_Data, A1),
    AROS_UFHA (APTR,              is_Code, A5),
    AROS_UFHA (struct ExecBase *, SysBase, A6)
)
{
    /* Disable(); */
    /* Check if a task switch is necessary */
    if(SysBase->TaskReady.lh_Head->ln_Succ!=NULL&&
       SysBase->ThisTask->tc_Node.ln_Pri<=
	((struct Task *)SysBase->TaskReady.lh_Head)->tc_Node.ln_Pri)
    {
	/* Check if it is possible */
	if(SysBase->TDNestCnt<0)
	{
	    if(SysBase->ThisTask->tc_State==TS_RUN)
	    {
		SysBase->ThisTask->tc_State=TS_READY;
		Enqueue(&SysBase->TaskReady,&SysBase->ThisTask->tc_Node);
		SysBase->AttnResched|=0x8000;
	    }
	}else
	    SysBase->AttnResched|=0x80;
    }
    /* Enable(); */
    /* Wasn't explicitly for me */
    return 0;
}

static APTR allocmem(ULONG size)
{
    UBYTE *ret;

    size = (size + MEMCHUNK_TOTAL-1) & ~(MEMCHUNK_TOTAL-1);

    ret = (UBYTE *)mh.mh_First;

    mh.mh_First = (struct MemChunk *)(ret + size);
    mh.mh_First->mc_Next = NULL;
    mh.mh_Free =
	mh.mh_First->mc_Bytes = ((struct MemChunk *)ret)->mc_Bytes - size;

    return ret;
}

void _aros_not_implemented (void)
{
    fprintf (stderr, "This function is not implemented\n");
}

void aros_print_not_implemented (char * name)
{
    fprintf (stderr, "The function %s is not implemented\n", name);
}

int main(int argc,char *argv[])
{
    ULONG * space;

    /* Put arguments into globals */
    gargc=argc;
    gargv=argv;

    /* Leave a space of 4096 bytes before the memory */
    space = malloc (4096);
#ifndef STATIC_MEMORY
    memory = malloc (MEMSIZE+MEMCHUNK_TOTAL);

    if (!memory)
    {
	fprintf (stderr, "Can't allocate RAM\n");
	exit (20);
    }
#endif

    { /* erase space */
	int size = 4096/sizeof(ULONG);

	while (--size)
	    *space ++ = 0xDEADBEEF;
    }

    /*
	Prepare first MemHeader. I cannot use exec functions
	here because exec is not yet up.
    */
    mh.mh_Node.ln_Name = "chip memory"; /* Amiga has always chip, but maybe no fast */
    mh.mh_Node.ln_Pri  = 0;
    mh.mh_Attributes   = MEMF_CHIP|MEMF_PUBLIC; /* Public to my emulation */
    mh.mh_First = (struct MemChunk *)
		(((IPTR)memory + MEMCHUNK_TOTAL-1) & ~(MEMCHUNK_TOTAL-1));
    mh.mh_First->mc_Next  = NULL;
    mh.mh_First->mc_Bytes = MEMSIZE;

    mh.mh_Lower = mh.mh_First;
    mh.mh_Upper = (UBYTE *)mh.mh_Lower+MEMSIZE;
    mh.mh_Free	= MEMSIZE;

    /* The following allocations cannot and must not fail. */
    {
	/* Prepare exec.library */
	ULONG neg, i;

	neg = AROS_ALIGN(LIB_VECTSIZE*NUMVECT);

	SysBase = (struct ExecBase *)
		((UBYTE *)allocmem (neg + sizeof (struct ExecBase)) + neg);

printf ("SysBase = %p\n", SysBase);

	for (i=1; i<=NUMVECT; i++)
	{
	    __AROS_INITVEC (SysBase, i);
	    __AROS_SETVECADDR (SysBase, i, ExecFunctions[i-1]);
	}

	SysBase->LibNode.lib_Node.ln_Name = "exec.library";
	SysBase->LibNode.lib_Version	  = 41;
	SysBase->LibNode.lib_Revision	  = 10;

	SysBase->DebugData = &AROSSupportBase;

	AROSSupportBase.kprintf = (void *)kprintf;

	NEWLIST (&SysBase->MemList);
	AddHead (&SysBase->MemList, &mh.mh_Node);
	NEWLIST (&SysBase->ResourceList);
	NEWLIST (&SysBase->DeviceList);
	NEWLIST (&SysBase->IntrList);
	NEWLIST (&SysBase->LibList);
	NEWLIST (&SysBase->PortList);
	NEWLIST (&SysBase->TaskReady);
	NEWLIST (&SysBase->TaskWait);

	for (i=0; i<5; i++)
	{
	    NEWLIST (&SysBase->SoftInts[i].sh_List);
	}

	for (i=0; i<16; i++)
	{
	    SysBase->IntVects[i].iv_Code = NULL;
	    SysBase->IntVects[i].iv_Data = NULL;
	    SysBase->IntVects[i].iv_Node = NULL;
	}

	NEWLIST (&SysBase->SemaphoreList);

	/* There are no memhandlers yet.
	 * (not even the library flushing one which is part of ram/dos.library) */
	NEWLIST ((struct List *)&SysBase->ex_MemHandlers);

	SysBase->IDNestCnt = 0;
	SysBase->TDNestCnt = 0;
	SysBase->AttnResched = 0;
    }

#ifndef _AMIGA
    if (mmap((APTR)0, getpagesize(), PROT_READ|PROT_WRITE,
	MAP_ANON|MAP_PRIVATE|MAP_FIXED, -1, 0) != (APTR)0)
      {
	perror("mmap: Can't map page zero");
	exit(10);
      }
    *(APTR *)4 = SysBase;
    if (mprotect((APTR)0, getpagesize(), PROT_READ)) {
	perror("mprotect");
	exit(10);
    }
#endif

    {
	/* Add boot task */
	struct Task    * t;
	struct MemList * ml;

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
	t->tc_SPLower = NULL;	    /* This is the system's stack */
	t->tc_SPUpper = (APTR)~0UL; /* all available addresses are ok */
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
	s =(UBYTE *)         AllocMem(AROS_STACKSIZE,         MEMF_PUBLIC|MEMF_CLEAR);
	ml->ml_NumEntries     =2;
	ml->ml_ME[0].me_Addr  =t;
	ml->ml_ME[0].me_Length=sizeof(struct Task);
	ml->ml_ME[1].me_Addr  =s;
	ml->ml_ME[1].me_Length=AROS_STACKSIZE;

	NEWLIST(&t->tc_MemEntry);
	AddHead(&t->tc_MemEntry,&ml->ml_Node);
	t->tc_SPLower=s;
	t->tc_SPUpper=s+AROS_STACKSIZE;
	t->tc_Node.ln_Name="Idle task";
	t->tc_Node.ln_Pri=-128;
	AddTask(t,&idleTask,NULL);
    }
    /* Install all interrupt servers */
    {
	int i;
	for(i=0;i<16;i++)
	    if((1<<i)&(INTF_PORTS|INTF_COPER|INTF_VERTB|INTF_EXTER|INTF_SETCLR))
	    {
		struct Interrupt *is;
		struct SoftIntList *sil;
		is=AllocMem(sizeof(struct Interrupt),MEMF_PUBLIC);
		sil=AllocMem(sizeof(struct SoftIntList),MEMF_PUBLIC);
		is->is_Code=&IntServer;
		is->is_Data=sil;
		NEWLIST((struct List *)sil);
		SetIntVector(i,is);
	    }
    }
    InitCore();
    /* Install the Dispatcher */
    {
	struct Interrupt *is;
	is=(struct Interrupt *)AllocMem(sizeof(struct Interrupt),MEMF_PUBLIC);
	is->is_Code=(void (*)())&Dispatcher;
	AddIntServer(INTB_VERTB,is);
    }
    Enable();
    Permit();

    debugmem ();

    (void) InitResident((struct Resident *)&Utility_resident,0);
    DOSBase = (struct DosLibrary *)InitResident((struct Resident *)&Dos_resident,0);

    (void) InitResident((struct Resident *)&Graphics_resident,0);
    (void) InitResident((struct Resident *)&Intuition_resident,0);
    (void) InitResident((struct Resident *)&Mathffp_resident,0);

    {
	struct consolebase
	{
	    struct Device device;
	};

	struct consolebase *conbase;

	conbase=(struct consolebase *)InitResident((struct Resident *)&Console_resident,0);
	/* AddDevice (&conbase->device); */
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
	    { NP_Entry,     (IPTR)boot },
	    { NP_Input,     (IPTR)MKBADDR(fh_stdin) },
	    { NP_Output,    (IPTR)MKBADDR(fh_stdout) },
	    { NP_Name,	    (IPTR)"Boot process" },
	    { NP_Cli,	    1 },
	    { TAG_END, }
	};

	emulbase = (struct emulbase *) InitResident (
	    (struct Resident *)&emul_handler_resident,
	    0
	);
	/* AddDevice (&emulbase->eb_device); */

	AssignLock ("C",    Lock ("SYS:c",    SHARED_LOCK));
	AssignLock ("S",    Lock ("SYS:s",    SHARED_LOCK));
	AssignLock ("Libs", Lock ("SYS:libs", SHARED_LOCK));
	AssignLock ("Devs", Lock ("SYS:devs", SHARED_LOCK));

	fh_stdin->fh_Device  =&emulbase->eb_device;
	fh_stdin->fh_Unit    =emulbase->eb_stdin;
	fh_stdout->fh_Device =&emulbase->eb_device;
	fh_stdout->fh_Unit   =emulbase->eb_stdout;

	/* AROSSupportBase.StdOut = MKBADDR(fh_stdout); */
	AROSSupportBase.StdOut = stderr;

	CreateNewProc (bootprocess);
    }

    {
	/* last thing before we go, make ^H the erase character */
	struct termios t;
	tcgetattr(0, &t);
	t.c_cc[VERASE] = '\b';
#ifndef TCSASOFT
#   define TCSASOFT 0
#endif
	tcsetattr(0, TCSANOW|TCSASOFT, &t);
    }
    RemTask(NULL); /* get rid of Boot task */
    Switch (); /* Rescedule */

    /* Should never get here... */
    return 21;
}
