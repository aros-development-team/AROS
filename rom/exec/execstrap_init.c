#include <exec/types.h>
#include <exec/resident.h>
#include <exec/nodes.h>
#include <exec/execbase.h>

#include <proto/exec.h>

#include "exec_extfuncs.h"

/*
    Architecture dependent function variations:
*/
extern void AROS_SLIB_ENTRY(GetCC_10,Exec)();
extern void AROS_SLIB_ENTRY(CacheClearU_20,Exec)();
extern void AROS_SLIB_ENTRY(CacheClearU_40,Exec)();

/*
    TODO:

    Check for the right mouse button, and disable AROSfA if depressed.
*/

int start(void)
{
    struct ExecBase *SysBase;
    register ULONG x, y;
    UWORD *color00 = (void *)0xdff180;
    UWORD cpuflags;

    /*
	High-tech display tricks (blue effects) :-)
    */
    for (x=0; x<1000; x++)
    {
	for (y = 200; y; y--) *color00 = 0x00f;
	for (y = 200; y; y--) *color00 = 0x000;
    }

    SysBase = *(void **)4;
    cpuflags = SysBase->AttnFlags;

    /*
	The biggie: SetFunction() as many library vectors as possible.
	Protection from multitasking is provided by SetFunction (Forbid/Permit).

	Some functions are safe to call even from interrupts, so protect these
	with Disable/Enable:

	Alert/Cause/Disable/Enable/FindName/FindPort/FindTask/PutMsg/ReplyMsg/Signal/
	AddHead/AddTail/Enqueue/RemHead/RemTail/Insert/Remove ... any more?
    */

    Disable();
    SetFunction((struct Library *)SysBase, ( 20 * -6), (APTR)&AROS_SLIB_ENTRY(Disable,Exec));
    SetFunction((struct Library *)SysBase, ( 22 * -6), (APTR)&AROS_SLIB_ENTRY(Forbid,Exec));
#if 0
    /* "Some trouble prevented CycleToMenu to initialize itself properly"
       Related to the microkernel */
    SetFunction((struct Library *)SysBase, ( 23 * -6), (APTR)&AROS_SLIB_ENTRY(Permit,Exec));
#endif
#if 0
    /*
	Any ixemul program that uses wildcards on the command line will crash the Amiga.
	Ixemul is responsible for this; there is a code-fragment in
	ixemul/library/__cli_parse.c to check out why our Insert() fails.
    */
    SetFunction((struct Library *)SysBase, ( 39 * -6), (APTR)&AROS_SLIB_ENTRY(Insert,Exec));
#endif
    SetFunction((struct Library *)SysBase, ( 40 * -6), (APTR)&AROS_SLIB_ENTRY(AddHead,Exec));
    SetFunction((struct Library *)SysBase, ( 41 * -6), (APTR)&AROS_SLIB_ENTRY(AddTail,Exec));
    SetFunction((struct Library *)SysBase, ( 42 * -6), (APTR)&AROS_SLIB_ENTRY(Remove,Exec));
    SetFunction((struct Library *)SysBase, ( 43 * -6), (APTR)&AROS_SLIB_ENTRY(RemHead,Exec));
    SetFunction((struct Library *)SysBase, ( 44 * -6), (APTR)&AROS_SLIB_ENTRY(RemTail,Exec));
    SetFunction((struct Library *)SysBase, ( 45 * -6), (APTR)&AROS_SLIB_ENTRY(Enqueue,Exec));

    SetFunction((struct Library *)SysBase, ( 46 * -6), (APTR)&AROS_SLIB_ENTRY(FindName,Exec));
    SetFunction((struct Library *)SysBase, ( 49 * -6), (APTR)&AROS_SLIB_ENTRY(FindTask,Exec));
    SetFunction((struct Library *)SysBase, ( 65 * -6), (APTR)&AROS_SLIB_ENTRY(FindPort,Exec));
    SetFunction((struct Library *)SysBase, ( 61 * -6), (APTR)&AROS_SLIB_ENTRY(PutMsg,Exec));

#if 0
    /* The "move.w ccr,d0" should really be implemented as part of the jumptable, for speed.
       Is this desirable? */
    if ((cpuflags & AFF_68010) == AFF_68010)
	SetFunction((struct Library *)SysBase, ( 88 * -6), (APTR)&AROS_SLIB_ENTRY(GetCC_10,Exec));
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
    if ((cpuflags & AFF_68020) == AFF_68020)
    {
	/* If 68040 is set, it implies 020 and 030 bits also set. */
	if ((cpuflags & AFF_68040) == AFF_68040)
	{
	    SetFunction((struct Library *)SysBase, (106 * -6), (APTR)&AROS_SLIB_ENTRY(CacheClearU_40,Exec));
	}
	else SetFunction((struct Library *)SysBase, (106 * -6), (APTR)&AROS_SLIB_ENTRY(CacheClearU_20,Exec));
    }
    else
    {
	/*
	    We are on a 68000/010. These have no caches, so this default call is
	    essentially a no-op (rts).
	*/
	SetFunction((struct Library *)SysBase, (106 * -6), (APTR)&AROS_SLIB_ENTRY(CacheClearU,Exec));
    }
    Enable();

    SetFunction((struct Library *)SysBase, ( 27 * -6), (APTR)&AROS_SLIB_ENTRY(SetIntVector,Exec));
    SetFunction((struct Library *)SysBase, ( 28 * -6), (APTR)&AROS_SLIB_ENTRY(AddIntServer,Exec));
    SetFunction((struct Library *)SysBase, ( 29 * -6), (APTR)&AROS_SLIB_ENTRY(RemIntServer,Exec));
    SetFunction((struct Library *)SysBase, ( 51 * -6), (APTR)&AROS_SLIB_ENTRY(SetSignal,Exec));
    SetFunction((struct Library *)SysBase, ( 59 * -6), (APTR)&AROS_SLIB_ENTRY(AddPort,Exec));
    SetFunction((struct Library *)SysBase, ( 60 * -6), (APTR)&AROS_SLIB_ENTRY(RemPort,Exec));
    SetFunction((struct Library *)SysBase, ( 62 * -6), (APTR)&AROS_SLIB_ENTRY(GetMsg,Exec));
    SetFunction((struct Library *)SysBase, ( 64 * -6), (APTR)&AROS_SLIB_ENTRY(WaitPort,Exec));
    SetFunction((struct Library *)SysBase, ( 66 * -6), (APTR)&AROS_SLIB_ENTRY(AddLibrary,Exec));
    SetFunction((struct Library *)SysBase, ( 67 * -6), (APTR)&AROS_SLIB_ENTRY(RemLibrary,Exec));
    SetFunction((struct Library *)SysBase, ( 68 * -6), (APTR)&AROS_SLIB_ENTRY(OldOpenLibrary,Exec));
#if 0
    /* Guru 01 00 00 0f: */
    SetFunction((struct Library *)SysBase, ( 69 * -6), (APTR)&AROS_SLIB_ENTRY(CloseLibrary,Exec));
#endif
#if 0
    /* Produces very strange code. "c:version" prints
       "Kickstart 39.106. Could not find version information for ''" and fails:
    */
    SetFunction((struct Library *)SysBase, ( 70 * -6), (APTR)&AROS_SLIB_ENTRY(SetFunction,Exec));
#endif
    SetFunction((struct Library *)SysBase, ( 71 * -6), (APTR)&AROS_SLIB_ENTRY(SumLibrary,Exec));
    SetFunction((struct Library *)SysBase, ( 72 * -6), (APTR)&AROS_SLIB_ENTRY(AddDevice,Exec));
    SetFunction((struct Library *)SysBase, ( 76 * -6), (APTR)&AROS_SLIB_ENTRY(DoIO,Exec));
    SetFunction((struct Library *)SysBase, ( 77 * -6), (APTR)&AROS_SLIB_ENTRY(SendIO,Exec));
    SetFunction((struct Library *)SysBase, ( 78 * -6), (APTR)&AROS_SLIB_ENTRY(CheckIO,Exec));
    SetFunction((struct Library *)SysBase, ( 79 * -6), (APTR)&AROS_SLIB_ENTRY(WaitIO,Exec));
    SetFunction((struct Library *)SysBase, ( 80 * -6), (APTR)&AROS_SLIB_ENTRY(AbortIO,Exec));
    SetFunction((struct Library *)SysBase, ( 81 * -6), (APTR)&AROS_SLIB_ENTRY(AddResource,Exec));
    SetFunction((struct Library *)SysBase, ( 82 * -6), (APTR)&AROS_SLIB_ENTRY(RemResource,Exec));
    SetFunction((struct Library *)SysBase, ( 83 * -6), (APTR)&AROS_SLIB_ENTRY(OpenResource,Exec));
    SetFunction((struct Library *)SysBase, ( 89 * -6), (APTR)&AROS_SLIB_ENTRY(TypeOfMem,Exec));
    SetFunction((struct Library *)SysBase, ( 92 * -6), (APTR)&AROS_SLIB_ENTRY(OpenLibrary,Exec));
    SetFunction((struct Library *)SysBase, ( 93 * -6), (APTR)&AROS_SLIB_ENTRY(InitSemaphore,Exec));
#if 0
    /* Can only be patched if we have control over the microkernel: */
    SetFunction((struct Library *)SysBase, ( 94 * -6), (APTR)&AROS_SLIB_ENTRY(ObtainSemaphore,Exec));
#endif
    SetFunction((struct Library *)SysBase, ( 96 * -6), (APTR)&AROS_SLIB_ENTRY(AttemptSemaphore,Exec));
    SetFunction((struct Library *)SysBase, ( 99 * -6), (APTR)&AROS_SLIB_ENTRY(FindSemaphore,Exec));
    SetFunction((struct Library *)SysBase, (100 * -6), (APTR)&AROS_SLIB_ENTRY(AddSemaphore,Exec));
    SetFunction((struct Library *)SysBase, (101 * -6), (APTR)&AROS_SLIB_ENTRY(RemSemaphore,Exec));
    SetFunction((struct Library *)SysBase, (103 * -6), (APTR)&AROS_SLIB_ENTRY(AddMemList,Exec));
    SetFunction((struct Library *)SysBase, (109 * -6), (APTR)&AROS_SLIB_ENTRY(CreateIORequest,Exec));
    SetFunction((struct Library *)SysBase, (110 * -6), (APTR)&AROS_SLIB_ENTRY(DeleteIORequest,Exec));
    SetFunction((struct Library *)SysBase, (111 * -6), (APTR)&AROS_SLIB_ENTRY(CreateMsgPort,Exec));
    SetFunction((struct Library *)SysBase, (112 * -6), (APTR)&AROS_SLIB_ENTRY(DeleteMsgPort,Exec));
#if 0
    /* Can only be patched if we have control over the microkernel: */
    SetFunction((struct Library *)SysBase, (113 * -6), (APTR)&AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec));
#endif
    SetFunction((struct Library *)SysBase, (120 * -6), (APTR)&AROS_SLIB_ENTRY(AttemptSemaphoreShared,Exec));
    SetFunction((struct Library *)SysBase, (129 * -6), (APTR)&AROS_SLIB_ENTRY(AddMemHandler,Exec));
    SetFunction((struct Library *)SysBase, (130 * -6), (APTR)&AROS_SLIB_ENTRY(RemMemHandler,Exec));
    SetFunction((struct Library *)SysBase, (135 * -6), (APTR)&AROS_SLIB_ENTRY(TaggedOpenLibrary,Exec));

    /*
	High-tech display tricks (green effects) :-)
    */
    for (x=0; x<1000; x++)
    {
	for (y = 200; y; y--) *color00 = 0x0f0;
	for (y = 200; y; y--) *color00 = 0x000;
    }

    return 0;
}

