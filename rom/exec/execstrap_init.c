/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Exec patches for AROS for Amiga
    Lang: english
*/

#undef DEBUG
#define DEBUG 0
#define LMBSUPPORT 1

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/nodes.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#if LMBSUPPORT > 0
#include <hardware/cia.h>
#endif

#include "exec_extfuncs.h"

#include <aros/debug.h>
#undef kprintf

/* flash() */
void flash(UWORD);

#define RED	0xf00
#define GREEN	0x0f0
#define BLUE	0x00f

/*
    Architecture dependent function variations:
*/
extern void AROS_SLIB_ENTRY(GetCC_10,Exec,88)();
extern void AROS_SLIB_ENTRY(CacheClearU_20,Exec,106)();
extern void AROS_SLIB_ENTRY(CacheClearU_40,Exec,106)();
extern void AROS_SLIB_ENTRY(CachePreDMA_40,Exec,127)();
extern void AROS_SLIB_ENTRY(CachePostDMA_30,Exec,128)();
extern void AROS_SLIB_ENTRY(CachePostDMA_40,Exec,128)();

/*
    TODO:

    Expand, improve and generally make the world a better place. :)
*/

int entry(void)
{
    return -1;
}

extern const char name[];
extern const char version[];
extern UBYTE dearray[];
extern int start(void);
extern const char end;

struct SpecialResident
{
    struct Resident res;
    ULONG magiccookie;
    UBYTE *statusarray;
    UWORD maxslot;
};

#define SR_COOKIE 0x4afa4afb

const struct SpecialResident resident =
{
    {
    RTC_MATCHWORD,
    (struct Resident*)&resident,
    (APTR)&end,
    RTF_COLDSTART,
    41, 		/* version */
    NT_KICKMEM,
    106,		/* Just above exec.library.
			   Because exec is RTF_SINGLETASK, and this is
			   RTF_COLDSTART, we'll still be started after
			   exec. */
    (char *)name,
    (char *)&version[6],
    &start
    },
    SR_COOKIE,		/* magic cookie to recognize a patchable library */
    dearray,		/* pointer to array of function status bytes */
    137 		/* highest vector slot in this library */
};

const char name[] = "exec.strap";
const char version[] = "$VER: exec.strap 41.10 (2.4.1997)";

/*
    Array of function slots to enable/disable. They are all set to 1 (enabled)
    by default. Arosboot will find the SR_COOKIE in the resident structure to see
    if this array is present, and will then disable certain functions if they
    are specified as off in the config file. We could extend this for more config
    file options, as we have 7 more bits to play with in the array.
*/
UBYTE dearray[] =
{
    /* 137 functions in exec.library V40 (plus one for offset 0) */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*   0-  9 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  10- 19 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  20- 29 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  30- 39 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  40- 49 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  50- 59 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  60- 69 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  70- 79 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  80- 89 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  90- 99 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 100-109 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 110-119 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 120-129 */
    1, 1, 1, 1, 1, 1, 1, 1	  /* 130-137 */
};

#define SetFunc(offset,name) \
{ \
    if(dearray[offset]) \
	SetFunction((struct Library *)SysBase, (offset * -6), (APTR)&AROS_SLIB_ENTRY(name,Exec,offset)); \
}

int start(void)
{
#if LMBSUPPORT > 0
    UBYTE *ciapra = (void *)0xbfe001;
#endif
    struct ExecBase *SysBase;
    UWORD cpuflags;

    SysBase = *(void **)4;
    cpuflags = SysBase->AttnFlags;

#if LMBSUPPORT > 0
    if(!(*ciapra & CIAF_GAMEPORT0))
    {
	D(bug("\nLMB pressed. Clearing reset vectors and resetting.\n\n"));
	SysBase->KickTagPtr = SysBase->KickMemPtr = SysBase->KickCheckSum = 0;
	flash(RED);
	ColdReboot(); /* Never returns. */
    }
#endif

    D(bug("\nexec.strap installing...\n"));

    /*
	This test will have to be changed if we start patching the exec version
	number.
    */
    if (SysBase->LibNode.lib_Version < 37)
    {
	/* Refuse to run on anything less than ROM 2.04 */
	D(bug("Found kickstart < 37. Exec.strap not started.\n"));
	return 0;
    }

    flash(BLUE);

#if 1 /* Disable in arosboot.config */
    /* The cause of a second infinite reset loop, as reported on 39.106 ROM?
       I have the same ROM, without this problem */
    /* Update: gcc produced 64-bit multiply instructions, which are missing on
       the 68060. Recoded so it won't produce these anymore. Still to test. */
    /* First patch SetFunction itself. */
    SetFunc( 70, SetFunction);
#endif

    /*
	The biggie: SetFunction() as many library vectors as possible.
	Protection from multitasking is provided by SetFunction (Forbid/Permit).

	Some functions are safe to call even from interrupts, so protect these
	with Disable/Enable:
	Alert/Cause/Disable/Enable/FindName/FindPort/FindTask/PutMsg/ReplyMsg/Signal/
	AddHead/AddTail/Enqueue/RemHead/RemTail/Insert/Remove ... any more?
    */
    Disable();

    SetFunc( 20, Disable);
    SetFunc( 22, Forbid);
#if 0
    /* "Some trouble prevented CycleToMenu to initialize itself properly"
       Related to the microkernel. I know what it is. Cannot be fixed right now. */
    SetFunc( 23, Permit);
#endif

    SetFunc( 39, Insert);
    SetFunc( 40, AddHead);
    SetFunc( 41, AddTail);
    SetFunc( 42, Remove);
    SetFunc( 43, RemHead);
    SetFunc( 44, RemTail);
#if 0
    /* Serial trouble. Can't receive a good/complete connectstring. */
    SetFunc( 45, Enqueue);
#endif
    SetFunc( 46, FindName);
    SetFunc( 49, FindTask);
    SetFunc( 65, FindPort);
    SetFunc( 61, PutMsg);
    SetFunc( 63, ReplyMsg);
#if 0
    /* The "move.w ccr,d0" should really be implemented as part of the jumptable, for speed.
       Do not patch, for now. */
    if ((cpuflags & AFF_68010) == AFF_68010)
	SetFunc( 88, GetCC_10);
#endif

#if 0
    /*
	BTW:  What bit(s) is (are) set for the MC68060?
	ANS:  Bit 7.
	BTW2: They would really be set by the 68060.library, which will obviously
	      not have executed at this point in the reset-procedure.
	ANS:  So we have to recognize it ourselves. Write routine.
	BTW3: If there is an agreed upon bit for the 68060, we could examine the
	      type of processor for ourselves in exec.strap, and update AttnFlags
	      accordingly.
	ANS:  See 2.
	BTW4: The 68060 can be recognized by its Processor Configuration Register (PCR).
	      This register also contains the bit to enable Superscalar Operation,
	      which we could set at this point in the reset-procedure to speed
	      things up considerably (if nothing breaks).
	ANS:  Just try it.
	BTW5: For the MC68060, we could also enable the Branch Cache at this point.
	ANS:  Yep.
    */

#warning TODO: Rework
    /* TODO: Rework this logic so a particular vector isn't patched more than
       once. SetFunction() calls CacheClearU(), so putting the routine address
       directly in the vector+2, and manually clearing of the cache (not with
       the Cachexxx() functions) when done is the way to go here. Better write
       a short assembler function to do all this.
    */
    SetFunc(106, CacheClearU);
    SetFunc(127, CachePreDMA);
    SetFunc(128, CachePostDMA);
    if ((cpuflags & AFF_68020) == AFF_68020)
    {
	SetFunc(106, CacheClearU_20);

	if ((cpuflags & AFF_68030) == AFF_68030)
	{
	    SetFunc(128, CachePostDMA_30);

	    if ((cpuflags & AFF_68040) == AFF_68040)
	    {
		SetFunc(106, CacheClearU_40);
		SetFunc(127, CachePreDMA_40);
		SetFunc(128, CachePostDMA_40);
	    }
	}
    }
#endif
    Enable();

    SetFunc( 12, InitCode);
#if 0
    /* Fails, presumably on the AROS_ALIGN restrictions: */
    SetFunc( 13, InitStruct);
#endif
    SetFunc( 14, MakeLibrary);
    SetFunc( 15, MakeFunctions);
    SetFunc( 16, FindResident);
    SetFunc( 17, InitResident);
    SetFunc( 27, SetIntVector);
    SetFunc( 28, AddIntServer);
    SetFunc( 29, RemIntServer);
#if 0
    /* Computer boots ok, but then programs fail. KingCON allows you to enter
       a command, but if you enter return, hangs. No other keypresses are
       accepted; mclk (clock window) still runs, though: */
    SetFunc( 33, AllocMem);
    SetFunc( 35, FreeMem);
#endif
    SetFunc( 36, AvailMem);
    SetFunc( 37, AllocEntry);
    SetFunc( 38, FreeEntry);
    SetFunc( 51, SetSignal);
    SetFunc( 55, AllocSignal);
    SetFunc( 56, FreeSignal);
    SetFunc( 59, AddPort);
    SetFunc( 60, RemPort);
    SetFunc( 62, GetMsg);
    SetFunc( 64, WaitPort);
    SetFunc( 66, AddLibrary);
    SetFunc( 67, RemLibrary);
    SetFunc( 68, OldOpenLibrary);
    SetFunc( 69, CloseLibrary);
    SetFunc( 71, SumLibrary);
    SetFunc( 72, AddDevice);
    SetFunc( 73, RemDevice);
    SetFunc( 74, OpenDevice);
    SetFunc( 75, CloseDevice);
    SetFunc( 76, DoIO);
    SetFunc( 77, SendIO);
    SetFunc( 78, CheckIO);
    SetFunc( 79, WaitIO);
    SetFunc( 80, AbortIO);
    SetFunc( 81, AddResource);
    SetFunc( 82, RemResource);
    SetFunc( 83, OpenResource);
#if 0
    /* Hangs just after accessing HD for the first time. Related to BCPL/BSTR
       handling? */
    SetFunc( 87, RawDoFmt);
#endif
    SetFunc( 89, TypeOfMem);
#if 1 /* Disable in arosboot.config */
    /* May be incompatible with OS37 OpenLibrary() */
    SetFunc( 92, OpenLibrary);
#endif
    SetFunc( 93, InitSemaphore);
#if 0
    /* Can only be patched if we have control over the microkernel: */
    SetFunc( 94, ObtainSemaphore);
#endif
#if 0 /* ZZZ */
    SetFunc( 96, AttemptSemaphore);
#endif
    SetFunc( 99, FindSemaphore);
    SetFunc(100, AddSemaphore);
    SetFunc(101, RemSemaphore);
    SetFunc(103, AddMemList);
    SetFunc(109, CreateIORequest);
    SetFunc(110, DeleteIORequest);
    SetFunc(111, CreateMsgPort);
    SetFunc(112, DeleteMsgPort);
#if 0
    /* Can only be patched if we have control over the microkernel: */
    SetFunc(113, ObtainSemaphoreShared);
#endif
    SetFunc(114, AllocVec);
    SetFunc(115, FreeVec);
#if 0 /* Enable as a group, as they are not internally compat with orig */
    /* Guru 0100000f-> "DH0 Software Failure"-> 80000003 guru upon reset */
    SetFunc(116, CreatePool);
    SetFunc(117, DeletePool);
    SetFunc(118, AllocPooled);
    SetFunc(119, FreePooled);
#endif
#if 0 /* ZZZ */
    SetFunc(120, AttemptSemaphoreShared);
#endif
    SetFunc(121, ColdReboot);

    /*
	This test will have to be changed if we start patching the exec version
	number.
    */
    if (SysBase->LibNode.lib_Version >= 39)
    {
	D(bug("Found kickstart >= 39. Extra functions installed.\n"));
	/* V39+ functions: */
	SetFunc(129, AddMemHandler);
	SetFunc(130, RemMemHandler);
	SetFunc(135, TaggedOpenLibrary);
    }
    /* We don't have to clear any caches, SetFunction takes care of them. */

    flash(GREEN);

    D(bug("exec.strap installation done.\n"));
    return 0;
}

/* High-tech display tricks :-) */
void flash(UWORD color)
{
    ULONG x, y;
    UWORD *color00 = (void *)0xdff180;

    for (x=0; x<1000; x++)
    {
	for (y = 200; y; y--) *color00 = color;
	for (y = 200; y; y--) *color00 = 0x000;
    }
}

const char end = 0;

/*************************************************************************
 *  Functions not yet added to this file (whether enabled or not):
 *
 *  Supervisor
 *  ExitIntr
 *  Schedule
 *  Reschedule
 *  Switch
 *  Dispatch
 *  Exception
 *  Alert
 *  Debug
 *  Enable
 *  SetSR
 *  SuperState
 *  UserState
 *  Cause
 *  Allocate
 *  Deallocate
 *  AllocAbs
 *  AddTask
 *  RemTask
 *  SetTaskPri
 *  SetExcept
 *  Wait
 *  Signal
 *  AllocTrap
 *  FreeTrap
 *  RawIOInit
 *  RawMayGetChar
 *  RawPutChar
 *  Procure
 *  Vacate
 *  ReleaseSemaphore
 *  ObtainSemaphoreList
 *  ReleaseSemaphoreList
 *  CopyMem
 *  CopyMemQuick
 *  CacheClearE
 *  CacheControl
 *  StackSwap
 *  ChildFree
 *  ChildOrphan
 *  ChildStatus
 *  ChildWait
 *  ObtainQuickVector
 *
 ************************************************************************/
