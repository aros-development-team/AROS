/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Exec vector table
    Lang: english
*/

#include <aros/libcall.h>
#include <exec/types.h>
#ifndef NULL
#define NULL ((void *)0)
#endif

void AROS_SLIB_ENTRY(open,Exec)();
void AROS_SLIB_ENTRY(close,Exec)();
void AROS_SLIB_ENTRY(null,Exec)();
void AROS_SLIB_ENTRY(Supervisor,Exec)();
void AROS_SLIB_ENTRY(Switch,Exec)();
void AROS_SLIB_ENTRY(Dispatch,Exec)();
void AROS_SLIB_ENTRY(Exception,Exec)();
void AROS_SLIB_ENTRY(PrepareContext,Exec)();
void AROS_SLIB_ENTRY(Reschedule,Exec)();
void AROS_SLIB_ENTRY(InitStruct,Exec)();
void AROS_SLIB_ENTRY(MakeLibrary,Exec)();
void AROS_SLIB_ENTRY(MakeFunctions,Exec)();
void AROS_SLIB_ENTRY(InitResident,Exec)();
void AROS_SLIB_ENTRY(Alert,Exec)();
void AROS_SLIB_ENTRY(Disable,Exec)();
void AROS_SLIB_ENTRY(Enable,Exec)();
void AROS_SLIB_ENTRY(Forbid,Exec)();
void AROS_SLIB_ENTRY(Permit,Exec)();
void AROS_SLIB_ENTRY(SetSR,Exec)();
void AROS_SLIB_ENTRY(SuperState,Exec)();
void AROS_SLIB_ENTRY(UserState,Exec)();
void AROS_SLIB_ENTRY(SetIntVector,Exec)();
void AROS_SLIB_ENTRY(AddIntServer,Exec)();
void AROS_SLIB_ENTRY(RemIntServer,Exec)();
void AROS_SLIB_ENTRY(Allocate,Exec)();
void AROS_SLIB_ENTRY(Deallocate,Exec)();
void AROS_SLIB_ENTRY(AllocMem,Exec)();
void AROS_SLIB_ENTRY(AllocAbs,Exec)();
void AROS_SLIB_ENTRY(FreeMem,Exec)();
void AROS_SLIB_ENTRY(AvailMem,Exec)();
void AROS_SLIB_ENTRY(AllocEntry,Exec)();
void AROS_SLIB_ENTRY(FreeEntry,Exec)();
void AROS_SLIB_ENTRY(Insert,Exec)();
void AROS_SLIB_ENTRY(AddHead,Exec)();
void AROS_SLIB_ENTRY(AddTail,Exec)();
void AROS_SLIB_ENTRY(Remove,Exec)();
void AROS_SLIB_ENTRY(RemHead,Exec)();
void AROS_SLIB_ENTRY(RemTail,Exec)();
void AROS_SLIB_ENTRY(Enqueue,Exec)();
void AROS_SLIB_ENTRY(FindName,Exec)();
void AROS_SLIB_ENTRY(AddTask,Exec)();
void AROS_SLIB_ENTRY(RemTask,Exec)();
void AROS_SLIB_ENTRY(FindTask,Exec)();
void AROS_SLIB_ENTRY(SetTaskPri,Exec)();
void AROS_SLIB_ENTRY(SetSignal,Exec)();
void AROS_SLIB_ENTRY(SetExcept,Exec)();
void AROS_SLIB_ENTRY(Wait,Exec)();
void AROS_SLIB_ENTRY(Signal,Exec)();
void AROS_SLIB_ENTRY(AllocSignal,Exec)();
void AROS_SLIB_ENTRY(FreeSignal,Exec)();
void AROS_SLIB_ENTRY(AddPort,Exec)();
void AROS_SLIB_ENTRY(RemPort,Exec)();
void AROS_SLIB_ENTRY(PutMsg,Exec)();
void AROS_SLIB_ENTRY(GetMsg,Exec)();
void AROS_SLIB_ENTRY(ReplyMsg,Exec)();
void AROS_SLIB_ENTRY(WaitPort,Exec)();
void AROS_SLIB_ENTRY(FindPort,Exec)();
void AROS_SLIB_ENTRY(AddLibrary,Exec)();
void AROS_SLIB_ENTRY(RemLibrary,Exec)();
void AROS_SLIB_ENTRY(OldOpenLibrary,Exec)();
void AROS_SLIB_ENTRY(CloseLibrary,Exec)();
void AROS_SLIB_ENTRY(SetFunction,Exec)();
void AROS_SLIB_ENTRY(SumLibrary,Exec)();
void AROS_SLIB_ENTRY(AddDevice,Exec)();
void AROS_SLIB_ENTRY(RemDevice,Exec)();
void AROS_SLIB_ENTRY(OpenDevice,Exec)();
void AROS_SLIB_ENTRY(CloseDevice,Exec)();
void AROS_SLIB_ENTRY(DoIO,Exec)();
void AROS_SLIB_ENTRY(SendIO,Exec)();
void AROS_SLIB_ENTRY(CheckIO,Exec)();
void AROS_SLIB_ENTRY(WaitIO,Exec)();
void AROS_SLIB_ENTRY(AbortIO,Exec)();
void AROS_SLIB_ENTRY(AddResource,Exec)();
void AROS_SLIB_ENTRY(RemResource,Exec)();
void AROS_SLIB_ENTRY(OpenResource,Exec)();
void AROS_SLIB_ENTRY(RawPutChar,Exec)();
void AROS_SLIB_ENTRY(RawDoFmt,Exec)();
void AROS_SLIB_ENTRY(TypeOfMem,Exec)();
void AROS_SLIB_ENTRY(Procure,Exec)();
void AROS_SLIB_ENTRY(Vacate,Exec)();
void AROS_SLIB_ENTRY(OpenLibrary,Exec)();
void AROS_SLIB_ENTRY(InitSemaphore,Exec)();
void AROS_SLIB_ENTRY(ObtainSemaphore,Exec)();
void AROS_SLIB_ENTRY(ReleaseSemaphore,Exec)();
void AROS_SLIB_ENTRY(AttemptSemaphore,Exec)();
void AROS_SLIB_ENTRY(ObtainSemaphoreList,Exec)();
void AROS_SLIB_ENTRY(ReleaseSemaphoreList,Exec)();
void AROS_SLIB_ENTRY(FindSemaphore,Exec)();
void AROS_SLIB_ENTRY(AddSemaphore,Exec)();
void AROS_SLIB_ENTRY(RemSemaphore,Exec)();
void AROS_SLIB_ENTRY(AddMemList,Exec)();
void AROS_SLIB_ENTRY(CopyMem,Exec)();
void AROS_SLIB_ENTRY(CopyMemQuick,Exec)();
void AROS_SLIB_ENTRY(CacheClearU,Exec)();
void AROS_SLIB_ENTRY(CacheClearE,Exec)();
void AROS_SLIB_ENTRY(CacheControl,Exec)();
void AROS_SLIB_ENTRY(CreateIORequest,Exec)();
void AROS_SLIB_ENTRY(DeleteIORequest,Exec)();
void AROS_SLIB_ENTRY(CreateMsgPort,Exec)();
void AROS_SLIB_ENTRY(DeleteMsgPort,Exec)();
void AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec)();
void AROS_SLIB_ENTRY(AllocVec,Exec)();
void AROS_SLIB_ENTRY(FreeVec,Exec)();
void AROS_SLIB_ENTRY(CreatePool,Exec)();
void AROS_SLIB_ENTRY(DeletePool,Exec)();
void AROS_SLIB_ENTRY(AllocPooled,Exec)();
void AROS_SLIB_ENTRY(FreePooled,Exec)();
void AROS_SLIB_ENTRY(AttemptSemaphoreShared,Exec)();
void AROS_SLIB_ENTRY(StackSwap,Exec)();
void AROS_SLIB_ENTRY(CachePreDMA,Exec)();
void AROS_SLIB_ENTRY(CachePostDMA,Exec)();
void AROS_SLIB_ENTRY(AddMemHandler,Exec)();
void AROS_SLIB_ENTRY(RemMemHandler,Exec)();
void AROS_SLIB_ENTRY(InitCode,Exec)();
void AROS_SLIB_ENTRY(FindResident,Exec)();
void AROS_SLIB_ENTRY(Debug,Exec)();
void AROS_SLIB_ENTRY(Cause,Exec)();
void AROS_SLIB_ENTRY(AllocTrap,Exec)();
void AROS_SLIB_ENTRY(FreeTrap,Exec)();
void AROS_SLIB_ENTRY(GetCC,Exec)();
void AROS_SLIB_ENTRY(SumKickData,Exec)();
void AROS_SLIB_ENTRY(ColdReboot,Exec)();
void AROS_SLIB_ENTRY(ChildFree,Exec)();
void AROS_SLIB_ENTRY(ChildOrphan,Exec)();
void AROS_SLIB_ENTRY(ChildStatus,Exec)();
void AROS_SLIB_ENTRY(ChildWait,Exec)();
void AROS_SLIB_ENTRY(ObtainQuickVector,Exec)();
void AROS_SLIB_ENTRY(RawIOInit,Exec)();
void AROS_SLIB_ENTRY(RawMayGetChar,Exec)();
void AROS_SLIB_ENTRY(RawPutChar,Exec)();
void AROS_SLIB_ENTRY(TaggedOpenLibrary,Exec)();
void AROS_SLIB_ENTRY(AllocVecPooled,Exec)();
void AROS_SLIB_ENTRY(FreeVecPooled,Exec)();
void AROS_SLIB_ENTRY(NewAllocEntry,Exec)();

const void *ExecFunctions[] __attribute__((section(".rodata"))) =
{
/*  1 */&AROS_SLIB_ENTRY(open,Exec),
	&AROS_SLIB_ENTRY(close,Exec),
	&AROS_SLIB_ENTRY(null,Exec),
	&AROS_SLIB_ENTRY(null,Exec),
	&AROS_SLIB_ENTRY(Supervisor,Exec),
	&AROS_SLIB_ENTRY(PrepareContext,Exec),
	NULL,		/* Private2 */
	&AROS_SLIB_ENTRY(Reschedule,Exec),
	&AROS_SLIB_ENTRY(Switch,Exec),
/* 10 */&AROS_SLIB_ENTRY(Dispatch,Exec),
	&AROS_SLIB_ENTRY(Exception,Exec),
	&AROS_SLIB_ENTRY(InitCode,Exec),
	&AROS_SLIB_ENTRY(InitStruct,Exec),
	&AROS_SLIB_ENTRY(MakeLibrary,Exec),
	&AROS_SLIB_ENTRY(MakeFunctions,Exec),
	&AROS_SLIB_ENTRY(FindResident,Exec),
	&AROS_SLIB_ENTRY(InitResident,Exec),
	&AROS_SLIB_ENTRY(Alert,Exec),
	&AROS_SLIB_ENTRY(Debug,Exec),
/* 20 */&AROS_SLIB_ENTRY(Disable,Exec),
	&AROS_SLIB_ENTRY(Enable,Exec),
	&AROS_SLIB_ENTRY(Forbid,Exec),
	&AROS_SLIB_ENTRY(Permit,Exec),
	&AROS_SLIB_ENTRY(SetSR,Exec),
	&AROS_SLIB_ENTRY(SuperState,Exec),
	&AROS_SLIB_ENTRY(UserState,Exec),
	&AROS_SLIB_ENTRY(SetIntVector,Exec),
	&AROS_SLIB_ENTRY(AddIntServer,Exec),
	&AROS_SLIB_ENTRY(RemIntServer,Exec),
/* 30 */&AROS_SLIB_ENTRY(Cause,Exec),
	&AROS_SLIB_ENTRY(Allocate,Exec),
	&AROS_SLIB_ENTRY(Deallocate,Exec),
	&AROS_SLIB_ENTRY(AllocMem,Exec),
	&AROS_SLIB_ENTRY(AllocAbs,Exec),
	&AROS_SLIB_ENTRY(FreeMem,Exec),
	&AROS_SLIB_ENTRY(AvailMem,Exec),
	&AROS_SLIB_ENTRY(AllocEntry,Exec),
	&AROS_SLIB_ENTRY(FreeEntry,Exec),
	&AROS_SLIB_ENTRY(Insert,Exec),
/* 40 */&AROS_SLIB_ENTRY(AddHead,Exec),
	&AROS_SLIB_ENTRY(AddTail,Exec),
	&AROS_SLIB_ENTRY(Remove,Exec),
	&AROS_SLIB_ENTRY(RemHead,Exec),
	&AROS_SLIB_ENTRY(RemTail,Exec),
	&AROS_SLIB_ENTRY(Enqueue,Exec),
	&AROS_SLIB_ENTRY(FindName,Exec),
	&AROS_SLIB_ENTRY(AddTask,Exec),
	&AROS_SLIB_ENTRY(RemTask,Exec),
	&AROS_SLIB_ENTRY(FindTask,Exec),
/* 50 */&AROS_SLIB_ENTRY(SetTaskPri,Exec),
	&AROS_SLIB_ENTRY(SetSignal,Exec),
	&AROS_SLIB_ENTRY(SetExcept,Exec),
	&AROS_SLIB_ENTRY(Wait,Exec),
	&AROS_SLIB_ENTRY(Signal,Exec),
	&AROS_SLIB_ENTRY(AllocSignal,Exec),
	&AROS_SLIB_ENTRY(FreeSignal,Exec),
	&AROS_SLIB_ENTRY(AllocTrap,Exec),
	&AROS_SLIB_ENTRY(FreeTrap,Exec),
	&AROS_SLIB_ENTRY(AddPort,Exec),
/* 60 */&AROS_SLIB_ENTRY(RemPort,Exec),
	&AROS_SLIB_ENTRY(PutMsg,Exec),
	&AROS_SLIB_ENTRY(GetMsg,Exec),
	&AROS_SLIB_ENTRY(ReplyMsg,Exec),
	&AROS_SLIB_ENTRY(WaitPort,Exec),
	&AROS_SLIB_ENTRY(FindPort,Exec),
	&AROS_SLIB_ENTRY(AddLibrary,Exec),
	&AROS_SLIB_ENTRY(RemLibrary,Exec),
	&AROS_SLIB_ENTRY(OldOpenLibrary,Exec),
	&AROS_SLIB_ENTRY(CloseLibrary,Exec),
/* 70 */&AROS_SLIB_ENTRY(SetFunction,Exec),
	&AROS_SLIB_ENTRY(SumLibrary,Exec),
	&AROS_SLIB_ENTRY(AddDevice,Exec),
	&AROS_SLIB_ENTRY(RemDevice,Exec),
	&AROS_SLIB_ENTRY(OpenDevice,Exec),
	&AROS_SLIB_ENTRY(CloseDevice,Exec),
	&AROS_SLIB_ENTRY(DoIO,Exec),
	&AROS_SLIB_ENTRY(SendIO,Exec),
	&AROS_SLIB_ENTRY(CheckIO,Exec),
	&AROS_SLIB_ENTRY(WaitIO,Exec),
/* 80 */&AROS_SLIB_ENTRY(AbortIO,Exec),
	&AROS_SLIB_ENTRY(AddResource,Exec),
	&AROS_SLIB_ENTRY(RemResource,Exec),
	&AROS_SLIB_ENTRY(OpenResource,Exec),
	&AROS_SLIB_ENTRY(RawIOInit,Exec), /* Private7 */
	&AROS_SLIB_ENTRY(RawMayGetChar,Exec), /* Private8 */
	&AROS_SLIB_ENTRY(RawPutChar,Exec),
	&AROS_SLIB_ENTRY(RawDoFmt,Exec),
	&AROS_SLIB_ENTRY(GetCC,Exec),
	&AROS_SLIB_ENTRY(TypeOfMem,Exec),
/* 90 */&AROS_SLIB_ENTRY(Procure,Exec),
	&AROS_SLIB_ENTRY(Vacate,Exec),
	&AROS_SLIB_ENTRY(OpenLibrary,Exec),
	&AROS_SLIB_ENTRY(InitSemaphore,Exec),
	&AROS_SLIB_ENTRY(ObtainSemaphore,Exec),
	&AROS_SLIB_ENTRY(ReleaseSemaphore,Exec),
	&AROS_SLIB_ENTRY(AttemptSemaphore,Exec),
	&AROS_SLIB_ENTRY(ObtainSemaphoreList,Exec),
	&AROS_SLIB_ENTRY(ReleaseSemaphoreList,Exec),
	&AROS_SLIB_ENTRY(FindSemaphore,Exec),
/*100 */&AROS_SLIB_ENTRY(AddSemaphore,Exec),
	&AROS_SLIB_ENTRY(RemSemaphore,Exec),
	&AROS_SLIB_ENTRY(SumKickData,Exec),
	&AROS_SLIB_ENTRY(AddMemList,Exec),
	&AROS_SLIB_ENTRY(CopyMem,Exec),
	&AROS_SLIB_ENTRY(CopyMemQuick,Exec),
	&AROS_SLIB_ENTRY(CacheClearU,Exec),
	&AROS_SLIB_ENTRY(CacheClearE,Exec),
	&AROS_SLIB_ENTRY(CacheControl,Exec),
	&AROS_SLIB_ENTRY(CreateIORequest,Exec),
/*110 */&AROS_SLIB_ENTRY(DeleteIORequest,Exec),
	&AROS_SLIB_ENTRY(CreateMsgPort,Exec),
	&AROS_SLIB_ENTRY(DeleteMsgPort,Exec),
	&AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec),
	&AROS_SLIB_ENTRY(AllocVec,Exec),
	&AROS_SLIB_ENTRY(FreeVec,Exec),
	&AROS_SLIB_ENTRY(CreatePool,Exec),
	&AROS_SLIB_ENTRY(DeletePool,Exec),
	&AROS_SLIB_ENTRY(AllocPooled,Exec),
	&AROS_SLIB_ENTRY(FreePooled,Exec),
/*120 */&AROS_SLIB_ENTRY(AttemptSemaphoreShared,Exec),
	&AROS_SLIB_ENTRY(ColdReboot,Exec),
	&AROS_SLIB_ENTRY(StackSwap,Exec),
	&AROS_SLIB_ENTRY(ChildFree,Exec),
	&AROS_SLIB_ENTRY(ChildOrphan,Exec),
	&AROS_SLIB_ENTRY(ChildStatus,Exec),
	&AROS_SLIB_ENTRY(ChildWait,Exec),
	&AROS_SLIB_ENTRY(CachePreDMA,Exec),
	&AROS_SLIB_ENTRY(CachePostDMA,Exec),
	&AROS_SLIB_ENTRY(AddMemHandler,Exec),
/*130 */&AROS_SLIB_ENTRY(RemMemHandler,Exec),
	&AROS_SLIB_ENTRY(ObtainQuickVector,Exec),
	NULL,
	NULL,
	NULL,
	&AROS_SLIB_ENTRY(TaggedOpenLibrary,Exec),
	NULL,
	NULL, /* 137 */
        NULL,
        NULL,
        NULL, /* 140 */
        NULL,
        NULL,
        NULL,
        NULL,
        NULL, /* 145 */
        NULL,
        NULL,
        NULL,
        &AROS_SLIB_ENTRY(AllocVecPooled,Exec), /* 149 */
        &AROS_SLIB_ENTRY(FreeVecPooled,Exec),  /* 150 */
        &AROS_SLIB_ENTRY(NewAllocEntry,Exec),  /* 151 */
	(APTR)-1
};
