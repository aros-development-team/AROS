/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/nodes.h>
#include <exec/execbase.h>
#include <hardware/cia.h>

#include <proto/exec.h>

#include "exec_extfuncs.h"

#define SetFunc(offset,name) \
    SetFunction((struct Library *)SysBase, (offset * -6), (APTR)&AROS_SLIB_ENTRY(name,Exec));

/*
    Architecture dependent function variations:
*/
extern void AROS_SLIB_ENTRY(GetCC_10,Exec)();
extern void AROS_SLIB_ENTRY(CacheClearU_20,Exec)();
extern void AROS_SLIB_ENTRY(CacheClearU_40,Exec)();
extern void AROS_SLIB_ENTRY(CachePreDMA_40,Exec)();
extern void AROS_SLIB_ENTRY(CachePostDMA_30,Exec)();
extern void AROS_SLIB_ENTRY(CachePostDMA_40,Exec)();

/*
    TODO:

    Expand, improve and generally make the world a better place. :)
*/

/*
    Make the colored stripes optional. There is a report that on the
    A4000/40, it never exits the stripes part of the code.
*/
#define FANCY_STRIPES 0

int entry(void)
{
    return 0;
}

extern const char name[];
extern const char version[];
extern int start(void);
extern const char end;

struct Resident resident =
{
    RTC_MATCHWORD,
    &resident,
    (APTR)&end,
    RTF_COLDSTART,
    1,			/* version */
    NT_KICKMEM,
    106,		/* Just above exec.library.
			   Because exec is RTF_SINGLETASK, and this is
			   RTF_COLDSTART, we'll still be started after
			   exec */
    (char *)name,
    (char *)&version[6],
    &start
};

const char name[] = "exec.strap";
const char version[] = "$VER: AROS exec.strap 41.4 (11.2.97)";

int start(void)
{
    struct ExecBase *SysBase;
#if (FANCY_STRIPES == 1)
    ULONG x, y;
    UWORD *color00 = (void *)0xdff180;
#endif
    UWORD cpuflags;

    if (SysBase->LibNode.lib_Version < 37)
    {
	/* Refuse to run on anything less than ROM 2.04 */
	return 0;
    }

#if (FANCY_STRIPES == 1)
    /* High-tech display tricks (blue effects) :-) */
    for (x=0; x<1000; x++)
    {
	for (y = 200; y; y--) *color00 = 0x00f;
	for (y = 200; y; y--) *color00 = 0x000;
    }
#endif

    SysBase = *(void **)4;
    cpuflags = SysBase->AttnFlags;

    /* First patch SetFunction itself. */
#if 0
    /* Produces very strange code. "c:version" prints
       "Kickstart 39.106. Could not find version information for ''" and fails:
    */
    /* Appears to generate correct code if compiled for 68000, and strange code
       if compiled for 68020+ */
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
    SetFunc( 45, Enqueue);
    SetFunc( 46, FindName);
    SetFunc( 49, FindTask);
    SetFunc( 65, FindPort);
    SetFunc( 61, PutMsg);
    SetFunc( 63, ReplyMsg);

#if 0
    /* The "move.w ccr,d0" should really be implemented as part of the jumptable, for speed.
       Is this desirable? */
    if ((cpuflags & AFF_68010) == AFF_68010)
	SetFunc( 88, GetCC_10);
#endif

    /*
       BTW:  What bit(s) is (are) set for the MC68060?
       BTW2: They would really be set by the 68060.library, which will obviously
             not have executed at this point in the reset-procedure.
       BTW3: If there is an agreed upon bit for the 68060, we could examine the
             type of processor for ourselves in exec.strap, and update AttnFlags
             accordingly.
       BTW4: The 68060 can be recognized by its Processor Configuration Register (PCR).
             This register also contains the bit to enable Superscalar Operation,
             which we could set at this point in the reset-procedure to speed
             things up considerably (if nothing breaks).
       BTW5: For the MC68060, we could also enable the Branch Cache at this point.
    */

    /* Pre-stuff these vectors for 68000/68010 use. */
    SetFunc(106, CacheClearU);
    SetFunc(127, CachePreDMA);
    SetFunc(128, CachePostDMA);

    /*
	Test for increasing processor types. We could also let these functions
	test ExecBase->AttnFlags for themselves and decide which action to take.
	That would mean less work here, but more work in the function that has
	to be repeated everytime it's called.
    */
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
#if 0
    /* "Could not mount PC0:": */
    SetFunc( 37, AllocEntry);
    /* Also disabled, as a dtor to AllocEntry */
    SetFunc( 38, FreeEntry);
#endif
    SetFunc( 51, SetSignal);
    SetFunc( 55, AllocSignal);
    SetFunc( 56, FreeSignal);
    SetFunc( 59, AddPort);
    SetFunc( 60, RemPort);
    SetFunc( 62, GetMsg);
#if 0
    /* Essentially works, but for some reason is not fast enough to detect a
       CONNECT string from my modem with ppp.device. Or something. */
    SetFunc( 64, WaitPort);
#endif
    SetFunc( 66, AddLibrary);
    SetFunc( 67, RemLibrary);
    SetFunc( 68, OldOpenLibrary);
#if 0
    /* Guru 01 00 00 0f (AN_BadFreeAddr): */
    SetFunc( 69, CloseLibrary);
#endif
    SetFunc( 71, SumLibrary);
    SetFunc( 72, AddDevice);
    SetFunc( 73, RemDevice);
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
    SetFunc( 92, OpenLibrary);
    SetFunc( 93, InitSemaphore);
#if 0
    /* Can only be patched if we have control over the microkernel: */
    SetFunc( 94, _ObtainSemaphore);
#endif
    SetFunc( 96, AttemptSemaphore);
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
    SetFunc(113, _ObtainSemaphoreShared);
#endif
    SetFunc(114, AllocVec);
    SetFunc(115, FreeVec);
    SetFunc(120, AttemptSemaphoreShared);

    if (SysBase->LibNode.lib_Version >= 39)
    {
	/* V39+ functions: */
	SetFunc(129, AddMemHandler);
	SetFunc(130, RemMemHandler);
	SetFunc(135, TaggedOpenLibrary);
    }
    /* We don't have to clear any caches, SetFunction takes care of them. */

#if (FANCY_STRIPES == 1)
    /*
	High-tech display tricks (green effects) :-)
    */
    for (x=0; x<1000; x++)
    {
	for (y = 200; y; y--) *color00 = 0x0f0;
	for (y = 200; y; y--) *color00 = 0x000;
    }
#endif

    return 0;
}

const char end = 0;
