#include <exec/types.h>
#include <exec/resident.h>
#include <exec/nodes.h>

#include <proto/exec.h>

#include "exec_extfuncs.h"

int start(void)
{
    struct ExecBase *SysBase;
    register ULONG x, y;
    UWORD *color00 = (void *)0xdff180;

    /*
	High-tech display tricks (blue effects) :-)
    */
    for (x=0; x<1000; x++)
    {
	for (y = 200; y; y--) *color00 = 0x00f;
	for (y = 200; y; y--) *color00 = 0x000;
    }

    SysBase = *(void **)4;

    /*
	The biggie: SetFunction() as many library vectors as possible. Protection
	from multitasking is provided by SetFunction (Forbid/Permit), but for
	safety also arbitrate for interrupts (Disable/Enable).
    */

    Disable();

#if 0
    /* Instant machine hang-up: */
    SetFunction((struct Library *)SysBase, ( 28 * -6), (APTR)&AROS_SLIB_ENTRY(AddIntServer,Exec));
    SetFunction((struct Library *)SysBase, ( 29 * -6), (APTR)&AROS_SLIB_ENTRY(RemIntServer,Exec));
#endif
#if 0
    /* E.g. grep gurus: */
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
    SetFunction((struct Library *)SysBase, ( 51 * -6), (APTR)&AROS_SLIB_ENTRY(SetSignal,Exec));
    SetFunction((struct Library *)SysBase, ( 59 * -6), (APTR)&AROS_SLIB_ENTRY(AddPort,Exec));
    SetFunction((struct Library *)SysBase, ( 60 * -6), (APTR)&AROS_SLIB_ENTRY(RemPort,Exec));
    SetFunction((struct Library *)SysBase, ( 61 * -6), (APTR)&AROS_SLIB_ENTRY(PutMsg,Exec));
    SetFunction((struct Library *)SysBase, ( 62 * -6), (APTR)&AROS_SLIB_ENTRY(GetMsg,Exec));
    SetFunction((struct Library *)SysBase, ( 64 * -6), (APTR)&AROS_SLIB_ENTRY(WaitPort,Exec));
    SetFunction((struct Library *)SysBase, ( 65 * -6), (APTR)&AROS_SLIB_ENTRY(FindPort,Exec));
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
#if 0
    /* Can only be patched if we have control over the microkernel: */
    SetFunction((struct Library *)SysBase, (113 * -6), (APTR)&AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec));
#endif
    SetFunction((struct Library *)SysBase, (120 * -6), (APTR)&AROS_SLIB_ENTRY(AttemptSemaphoreShared,Exec));
    SetFunction((struct Library *)SysBase, (129 * -6), (APTR)&AROS_SLIB_ENTRY(AddMemHandler,Exec));
    SetFunction((struct Library *)SysBase, (130 * -6), (APTR)&AROS_SLIB_ENTRY(RemMemHandler,Exec));
    SetFunction((struct Library *)SysBase, (135 * -6), (APTR)&AROS_SLIB_ENTRY(TaggedOpenLibrary,Exec));

    Enable();

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

