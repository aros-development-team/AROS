/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.9  1996/10/24 15:50:48  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.8  1996/10/23 14:27:37  aros
    Space/Tab conversion by XDME :-)

    Revision 1.7  1996/10/14 11:17:26  digulla
    Use AROS_SLIB_ENTRY

    Revision 1.6  1996/10/10 13:22:49  digulla
    New function: RawPutChar,Exec)()

    Revision 1.5  1996/09/11 16:54:24  digulla
    Always use AROS_SLIB_ENTRY,Exec)() to access shared external symbols, because
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

void AROS_SLIB_ENTRY(Supervisor,Exec)();
void AROS_SLIB_ENTRY(Switch,Exec)();
void AROS_SLIB_ENTRY(Dispatch,Exec)();
void AROS_SLIB_ENTRY(Exception,Exec)();
void AROS_SLIB_ENTRY(PrepareContext,Exec)();
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
void AROS_SLIB_ENTRY(_ObtainSemaphore,Exec)();
void AROS_SLIB_ENTRY(_ReleaseSemaphore,Exec)();
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
void AROS_SLIB_ENTRY(_ObtainSemaphoreShared,Exec)();
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

void *ExecFunctions[131]=
{
/*  1 */NULL,
	NULL,
	NULL,
	NULL,
	&AROS_SLIB_ENTRY(Supervisor,Exec),
	&AROS_SLIB_ENTRY(Switch,Exec),
	&AROS_SLIB_ENTRY(Dispatch,Exec),
	&AROS_SLIB_ENTRY(Exception,Exec),
	&AROS_SLIB_ENTRY(PrepareContext,Exec),
/* 10 */NULL,		/* Private5 */
	NULL,		/* Private6 */
	NULL,		/* InitCode */
	&AROS_SLIB_ENTRY(InitStruct,Exec),
	&AROS_SLIB_ENTRY(MakeLibrary,Exec),
	&AROS_SLIB_ENTRY(MakeFunctions,Exec),
	NULL,		/* FindResident */
	&AROS_SLIB_ENTRY(InitResident,Exec),
	&AROS_SLIB_ENTRY(Alert,Exec),
	NULL,		/* Debug */
/* 20 */&AROS_SLIB_ENTRY(Disable,Exec),
	&AROS_SLIB_ENTRY(Enable,Exec),
	&AROS_SLIB_ENTRY(Forbid,Exec),
	&AROS_SLIB_ENTRY(Permit,Exec),
	&AROS_SLIB_ENTRY(SetSR,Exec),
	&AROS_SLIB_ENTRY(SuperState,Exec),
	&AROS_SLIB_ENTRY(UserState,Exec),
	NULL,		/* SetIntVector */
	NULL,		/* AddIntServer */
	NULL,		/* RemIntServer */
/* 30 */NULL,		/* Cause */
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
	NULL,		/* AllocTrap */
	NULL,		/* FreeTrap */
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
	NULL,		/* Private7 */
	NULL,		/* Private8 */
	&AROS_SLIB_ENTRY(RawPutChar,Exec),
	&AROS_SLIB_ENTRY(RawDoFmt,Exec),
	NULL,		/* GetCC */
	&AROS_SLIB_ENTRY(TypeOfMem,Exec),
/* 90 */&AROS_SLIB_ENTRY(Procure,Exec),
	&AROS_SLIB_ENTRY(Vacate,Exec),
	&AROS_SLIB_ENTRY(OpenLibrary,Exec),
	&AROS_SLIB_ENTRY(InitSemaphore,Exec),
	&AROS_SLIB_ENTRY(_ObtainSemaphore,Exec),
	&AROS_SLIB_ENTRY(_ReleaseSemaphore,Exec),
	&AROS_SLIB_ENTRY(AttemptSemaphore,Exec),
	&AROS_SLIB_ENTRY(ObtainSemaphoreList,Exec),
	&AROS_SLIB_ENTRY(ReleaseSemaphoreList,Exec),
	&AROS_SLIB_ENTRY(FindSemaphore,Exec),
/*100 */&AROS_SLIB_ENTRY(AddSemaphore,Exec),
	&AROS_SLIB_ENTRY(RemSemaphore,Exec),
	NULL,		/* SumKickData */
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
	&AROS_SLIB_ENTRY(_ObtainSemaphoreShared,Exec),
	&AROS_SLIB_ENTRY(AllocVec,Exec),
	&AROS_SLIB_ENTRY(FreeVec,Exec),
	&AROS_SLIB_ENTRY(CreatePool,Exec),
	&AROS_SLIB_ENTRY(DeletePool,Exec),
	&AROS_SLIB_ENTRY(AllocPooled,Exec),
	&AROS_SLIB_ENTRY(FreePooled,Exec),
/*120 */&AROS_SLIB_ENTRY(AttemptSemaphoreShared,Exec),
	NULL,		/* ColdReboot */
	&AROS_SLIB_ENTRY(StackSwap,Exec),
	NULL,		/* ChildFree */
	NULL,		/* ChildOrphan */
	NULL,		/* ChildStatus */
	NULL,		/* ChildWait */
	&AROS_SLIB_ENTRY(CachePreDMA,Exec),
	&AROS_SLIB_ENTRY(CachePostDMA,Exec),
	&AROS_SLIB_ENTRY(AddMemHandler,Exec),
/*130 */&AROS_SLIB_ENTRY(RemMemHandler,Exec),
	NULL		/* ObtainQuickVector */
};
