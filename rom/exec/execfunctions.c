/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1996/10/14 11:17:26  digulla
    Use __AROS_SLIB_ENTRY

    Revision 1.6  1996/10/10 13:22:49  digulla
    New function: RawPutChar,Exec)()

    Revision 1.5  1996/09/11 16:54:24  digulla
    Always use __AROS_SLIB_ENTRY,Exec)() to access shared external symbols, because
    	some systems name an external symbol "x" as "_x" and others as "x".
    	(The problem arises with assembler symbols which might differ)

    Revision 1.4  1996/08/23 17:07:22  digulla
    The number of functions is hardcoded in init.c, so it should be the same here.

    Revision 1.3  1996/08/01 17:41:10  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <aros/libcall.h>
#ifndef NULL
#define NULL ((void *)0)
#endif

void __AROS_SLIB_ENTRY(Supervisor,Exec)();
void __AROS_SLIB_ENTRY(Switch,Exec)();
void __AROS_SLIB_ENTRY(Dispatch,Exec)();
void __AROS_SLIB_ENTRY(Exception,Exec)();
void __AROS_SLIB_ENTRY(PrepareContext,Exec)();
void __AROS_SLIB_ENTRY(InitStruct,Exec)();
void __AROS_SLIB_ENTRY(MakeLibrary,Exec)();
void __AROS_SLIB_ENTRY(MakeFunctions,Exec)();
void __AROS_SLIB_ENTRY(InitResident,Exec)();
void __AROS_SLIB_ENTRY(Alert,Exec)();
void __AROS_SLIB_ENTRY(Disable,Exec)();
void __AROS_SLIB_ENTRY(Enable,Exec)();
void __AROS_SLIB_ENTRY(Forbid,Exec)();
void __AROS_SLIB_ENTRY(Permit,Exec)();
void __AROS_SLIB_ENTRY(SetSR,Exec)();
void __AROS_SLIB_ENTRY(SuperState,Exec)();
void __AROS_SLIB_ENTRY(UserState,Exec)();
void __AROS_SLIB_ENTRY(Allocate,Exec)();
void __AROS_SLIB_ENTRY(Deallocate,Exec)();
void __AROS_SLIB_ENTRY(AllocMem,Exec)();
void __AROS_SLIB_ENTRY(AllocAbs,Exec)();
void __AROS_SLIB_ENTRY(FreeMem,Exec)();
void __AROS_SLIB_ENTRY(AvailMem,Exec)();
void __AROS_SLIB_ENTRY(AllocEntry,Exec)();
void __AROS_SLIB_ENTRY(FreeEntry,Exec)();
void __AROS_SLIB_ENTRY(Insert,Exec)();
void __AROS_SLIB_ENTRY(AddHead,Exec)();
void __AROS_SLIB_ENTRY(AddTail,Exec)();
void __AROS_SLIB_ENTRY(Remove,Exec)();
void __AROS_SLIB_ENTRY(RemHead,Exec)();
void __AROS_SLIB_ENTRY(RemTail,Exec)();
void __AROS_SLIB_ENTRY(Enqueue,Exec)();
void __AROS_SLIB_ENTRY(FindName,Exec)();
void __AROS_SLIB_ENTRY(AddTask,Exec)();
void __AROS_SLIB_ENTRY(RemTask,Exec)();
void __AROS_SLIB_ENTRY(FindTask,Exec)();
void __AROS_SLIB_ENTRY(SetTaskPri,Exec)();
void __AROS_SLIB_ENTRY(SetSignal,Exec)();
void __AROS_SLIB_ENTRY(SetExcept,Exec)();
void __AROS_SLIB_ENTRY(Wait,Exec)();
void __AROS_SLIB_ENTRY(Signal,Exec)();
void __AROS_SLIB_ENTRY(AllocSignal,Exec)();
void __AROS_SLIB_ENTRY(FreeSignal,Exec)();
void __AROS_SLIB_ENTRY(AddPort,Exec)();
void __AROS_SLIB_ENTRY(RemPort,Exec)();
void __AROS_SLIB_ENTRY(PutMsg,Exec)();
void __AROS_SLIB_ENTRY(GetMsg,Exec)();
void __AROS_SLIB_ENTRY(ReplyMsg,Exec)();
void __AROS_SLIB_ENTRY(WaitPort,Exec)();
void __AROS_SLIB_ENTRY(FindPort,Exec)();
void __AROS_SLIB_ENTRY(AddLibrary,Exec)();
void __AROS_SLIB_ENTRY(RemLibrary,Exec)();
void __AROS_SLIB_ENTRY(OldOpenLibrary,Exec)();
void __AROS_SLIB_ENTRY(CloseLibrary,Exec)();
void __AROS_SLIB_ENTRY(SetFunction,Exec)();
void __AROS_SLIB_ENTRY(SumLibrary,Exec)();
void __AROS_SLIB_ENTRY(AddDevice,Exec)();
void __AROS_SLIB_ENTRY(RemDevice,Exec)();
void __AROS_SLIB_ENTRY(OpenDevice,Exec)();
void __AROS_SLIB_ENTRY(CloseDevice,Exec)();
void __AROS_SLIB_ENTRY(DoIO,Exec)();
void __AROS_SLIB_ENTRY(SendIO,Exec)();
void __AROS_SLIB_ENTRY(CheckIO,Exec)();
void __AROS_SLIB_ENTRY(WaitIO,Exec)();
void __AROS_SLIB_ENTRY(AbortIO,Exec)();
void __AROS_SLIB_ENTRY(AddResource,Exec)();
void __AROS_SLIB_ENTRY(RemResource,Exec)();
void __AROS_SLIB_ENTRY(OpenResource,Exec)();
void __AROS_SLIB_ENTRY(RawPutChar,Exec)();
void __AROS_SLIB_ENTRY(RawDoFmt,Exec)();
void __AROS_SLIB_ENTRY(TypeOfMem,Exec)();
void __AROS_SLIB_ENTRY(Procure,Exec)();
void __AROS_SLIB_ENTRY(Vacate,Exec)();
void __AROS_SLIB_ENTRY(OpenLibrary,Exec)();
void __AROS_SLIB_ENTRY(InitSemaphore,Exec)();
void __AROS_SLIB_ENTRY(_ObtainSemaphore,Exec)();
void __AROS_SLIB_ENTRY(_ReleaseSemaphore,Exec)();
void __AROS_SLIB_ENTRY(AttemptSemaphore,Exec)();
void __AROS_SLIB_ENTRY(ObtainSemaphoreList,Exec)();
void __AROS_SLIB_ENTRY(ReleaseSemaphoreList,Exec)();
void __AROS_SLIB_ENTRY(FindSemaphore,Exec)();
void __AROS_SLIB_ENTRY(AddSemaphore,Exec)();
void __AROS_SLIB_ENTRY(RemSemaphore,Exec)();
void __AROS_SLIB_ENTRY(AddMemList,Exec)();
void __AROS_SLIB_ENTRY(CopyMem,Exec)();
void __AROS_SLIB_ENTRY(CopyMemQuick,Exec)();
void __AROS_SLIB_ENTRY(CacheClearU,Exec)();
void __AROS_SLIB_ENTRY(CacheClearE,Exec)();
void __AROS_SLIB_ENTRY(CacheControl,Exec)();
void __AROS_SLIB_ENTRY(CreateIORequest,Exec)();
void __AROS_SLIB_ENTRY(DeleteIORequest,Exec)();
void __AROS_SLIB_ENTRY(CreateMsgPort,Exec)();
void __AROS_SLIB_ENTRY(DeleteMsgPort,Exec)();
void __AROS_SLIB_ENTRY(_ObtainSemaphoreShared,Exec)();
void __AROS_SLIB_ENTRY(AllocVec,Exec)();
void __AROS_SLIB_ENTRY(FreeVec,Exec)();
void __AROS_SLIB_ENTRY(CreatePool,Exec)();
void __AROS_SLIB_ENTRY(DeletePool,Exec)();
void __AROS_SLIB_ENTRY(AllocPooled,Exec)();
void __AROS_SLIB_ENTRY(FreePooled,Exec)();
void __AROS_SLIB_ENTRY(AttemptSemaphoreShared,Exec)();
void __AROS_SLIB_ENTRY(StackSwap,Exec)();
void __AROS_SLIB_ENTRY(CachePreDMA,Exec)();
void __AROS_SLIB_ENTRY(CachePostDMA,Exec)();
void __AROS_SLIB_ENTRY(AddMemHandler,Exec)();
void __AROS_SLIB_ENTRY(RemMemHandler,Exec)();

void *ExecFunctions[131]=
{
/*  1 */NULL,
	NULL,
	NULL,
	NULL,
	&__AROS_SLIB_ENTRY(Supervisor,Exec),
	&__AROS_SLIB_ENTRY(Switch,Exec),
	&__AROS_SLIB_ENTRY(Dispatch,Exec),
	&__AROS_SLIB_ENTRY(Exception,Exec),
	&__AROS_SLIB_ENTRY(PrepareContext,Exec),
/* 10 */NULL,		/* Private5 */
	NULL,		/* Private6 */
	NULL,		/* InitCode */
	&__AROS_SLIB_ENTRY(InitStruct,Exec),
	&__AROS_SLIB_ENTRY(MakeLibrary,Exec),
	&__AROS_SLIB_ENTRY(MakeFunctions,Exec),
	NULL,		/* FindResident */
	&__AROS_SLIB_ENTRY(InitResident,Exec),
	&__AROS_SLIB_ENTRY(Alert,Exec),
	NULL,		/* Debug */
/* 20 */&__AROS_SLIB_ENTRY(Disable,Exec),
	&__AROS_SLIB_ENTRY(Enable,Exec),
	&__AROS_SLIB_ENTRY(Forbid,Exec),
	&__AROS_SLIB_ENTRY(Permit,Exec),
	&__AROS_SLIB_ENTRY(SetSR,Exec),
	&__AROS_SLIB_ENTRY(SuperState,Exec),
	&__AROS_SLIB_ENTRY(UserState,Exec),
	NULL,		/* SetIntVector */
	NULL,		/* AddIntServer */
	NULL,		/* RemIntServer */
/* 30 */NULL,		/* Cause */
	&__AROS_SLIB_ENTRY(Allocate,Exec),
	&__AROS_SLIB_ENTRY(Deallocate,Exec),
	&__AROS_SLIB_ENTRY(AllocMem,Exec),
	&__AROS_SLIB_ENTRY(AllocAbs,Exec),
	&__AROS_SLIB_ENTRY(FreeMem,Exec),
	&__AROS_SLIB_ENTRY(AvailMem,Exec),
	&__AROS_SLIB_ENTRY(AllocEntry,Exec),
	&__AROS_SLIB_ENTRY(FreeEntry,Exec),
	&__AROS_SLIB_ENTRY(Insert,Exec),
/* 40 */&__AROS_SLIB_ENTRY(AddHead,Exec),
	&__AROS_SLIB_ENTRY(AddTail,Exec),
	&__AROS_SLIB_ENTRY(Remove,Exec),
	&__AROS_SLIB_ENTRY(RemHead,Exec),
	&__AROS_SLIB_ENTRY(RemTail,Exec),
	&__AROS_SLIB_ENTRY(Enqueue,Exec),
	&__AROS_SLIB_ENTRY(FindName,Exec),
	&__AROS_SLIB_ENTRY(AddTask,Exec),
	&__AROS_SLIB_ENTRY(RemTask,Exec),
	&__AROS_SLIB_ENTRY(FindTask,Exec),
/* 50 */&__AROS_SLIB_ENTRY(SetTaskPri,Exec),
	&__AROS_SLIB_ENTRY(SetSignal,Exec),
	&__AROS_SLIB_ENTRY(SetExcept,Exec),
	&__AROS_SLIB_ENTRY(Wait,Exec),
	&__AROS_SLIB_ENTRY(Signal,Exec),
	&__AROS_SLIB_ENTRY(AllocSignal,Exec),
	&__AROS_SLIB_ENTRY(FreeSignal,Exec),
	NULL,		/* AllocTrap */
	NULL,		/* FreeTrap */
	&__AROS_SLIB_ENTRY(AddPort,Exec),
/* 60 */&__AROS_SLIB_ENTRY(RemPort,Exec),
	&__AROS_SLIB_ENTRY(PutMsg,Exec),
	&__AROS_SLIB_ENTRY(GetMsg,Exec),
	&__AROS_SLIB_ENTRY(ReplyMsg,Exec),
	&__AROS_SLIB_ENTRY(WaitPort,Exec),
	&__AROS_SLIB_ENTRY(FindPort,Exec),
	&__AROS_SLIB_ENTRY(AddLibrary,Exec),
	&__AROS_SLIB_ENTRY(RemLibrary,Exec),
	&__AROS_SLIB_ENTRY(OldOpenLibrary,Exec),
	&__AROS_SLIB_ENTRY(CloseLibrary,Exec),
/* 70 */&__AROS_SLIB_ENTRY(SetFunction,Exec),
	&__AROS_SLIB_ENTRY(SumLibrary,Exec),
	&__AROS_SLIB_ENTRY(AddDevice,Exec),
	&__AROS_SLIB_ENTRY(RemDevice,Exec),
	&__AROS_SLIB_ENTRY(OpenDevice,Exec),
	&__AROS_SLIB_ENTRY(CloseDevice,Exec),
	&__AROS_SLIB_ENTRY(DoIO,Exec),
	&__AROS_SLIB_ENTRY(SendIO,Exec),
	&__AROS_SLIB_ENTRY(CheckIO,Exec),
	&__AROS_SLIB_ENTRY(WaitIO,Exec),
/* 80 */&__AROS_SLIB_ENTRY(AbortIO,Exec),
	&__AROS_SLIB_ENTRY(AddResource,Exec),
	&__AROS_SLIB_ENTRY(RemResource,Exec),
	&__AROS_SLIB_ENTRY(OpenResource,Exec),
	NULL,		/* Private7 */
	NULL,		/* Private8 */
	&__AROS_SLIB_ENTRY(RawPutChar,Exec),
	&__AROS_SLIB_ENTRY(RawDoFmt,Exec),
	NULL,		/* GetCC */
	&__AROS_SLIB_ENTRY(TypeOfMem,Exec),
/* 90 */&__AROS_SLIB_ENTRY(Procure,Exec),
	&__AROS_SLIB_ENTRY(Vacate,Exec),
	&__AROS_SLIB_ENTRY(OpenLibrary,Exec),
	&__AROS_SLIB_ENTRY(InitSemaphore,Exec),
	&__AROS_SLIB_ENTRY(_ObtainSemaphore,Exec),
	&__AROS_SLIB_ENTRY(_ReleaseSemaphore,Exec),
	&__AROS_SLIB_ENTRY(AttemptSemaphore,Exec),
	&__AROS_SLIB_ENTRY(ObtainSemaphoreList,Exec),
	&__AROS_SLIB_ENTRY(ReleaseSemaphoreList,Exec),
	&__AROS_SLIB_ENTRY(FindSemaphore,Exec),
/*100 */&__AROS_SLIB_ENTRY(AddSemaphore,Exec),
	&__AROS_SLIB_ENTRY(RemSemaphore,Exec),
	NULL,		/* SumKickData */
	&__AROS_SLIB_ENTRY(AddMemList,Exec),
	&__AROS_SLIB_ENTRY(CopyMem,Exec),
	&__AROS_SLIB_ENTRY(CopyMemQuick,Exec),
	&__AROS_SLIB_ENTRY(CacheClearU,Exec),
	&__AROS_SLIB_ENTRY(CacheClearE,Exec),
	&__AROS_SLIB_ENTRY(CacheControl,Exec),
	&__AROS_SLIB_ENTRY(CreateIORequest,Exec),
/*110 */&__AROS_SLIB_ENTRY(DeleteIORequest,Exec),
	&__AROS_SLIB_ENTRY(CreateMsgPort,Exec),
	&__AROS_SLIB_ENTRY(DeleteMsgPort,Exec),
	&__AROS_SLIB_ENTRY(_ObtainSemaphoreShared,Exec),
	&__AROS_SLIB_ENTRY(AllocVec,Exec),
	&__AROS_SLIB_ENTRY(FreeVec,Exec),
	&__AROS_SLIB_ENTRY(CreatePool,Exec),
	&__AROS_SLIB_ENTRY(DeletePool,Exec),
	&__AROS_SLIB_ENTRY(AllocPooled,Exec),
	&__AROS_SLIB_ENTRY(FreePooled,Exec),
/*120 */&__AROS_SLIB_ENTRY(AttemptSemaphoreShared,Exec),
	NULL,		/* ColdReboot */
	&__AROS_SLIB_ENTRY(StackSwap,Exec),
	NULL,		/* ChildFree */
	NULL,		/* ChildOrphan */
	NULL,		/* ChildStatus */
	NULL,		/* ChildWait */
	&__AROS_SLIB_ENTRY(CachePreDMA,Exec),
	&__AROS_SLIB_ENTRY(CachePostDMA,Exec),
	&__AROS_SLIB_ENTRY(AddMemHandler,Exec),
/*130 */&__AROS_SLIB_ENTRY(RemMemHandler,Exec),
	NULL		/* ObtainQuickVector */
};
