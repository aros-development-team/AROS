/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/09/11 16:54:24  digulla
    Always use __AROS_SLIB_ENTRY() to access shared external symbols, because
    	some systems name an external symbol "x" as "_x" and others as "x".
    	(The problem arises with assembler symbols which might differ)

    Revision 1.4  1996/08/23 17:07:22  digulla
    The number of functions is hardcoded in init.c, so it should be the same here.

    Revision 1.3  1996/08/01 17:41:10  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#ifndef NULL
#define NULL ((void *)0)
#endif

void _Exec_Supervisor();
void _Exec_Switch();
void _Exec_Dispatch();
void _Exec_Exception();
void _Exec_PrepareContext();
void _Exec_InitStruct();
void _Exec_MakeLibrary();
void _Exec_MakeFunctions();
void _Exec_InitResident();
void _Exec_Alert();
void _Exec_Disable();
void _Exec_Enable();
void _Exec_Forbid();
void _Exec_Permit();
void _Exec_SetSR();
void _Exec_SuperState();
void _Exec_UserState();
void _Exec_Allocate();
void _Exec_Deallocate();
void _Exec_AllocMem();
void _Exec_AllocAbs();
void _Exec_FreeMem();
void _Exec_AvailMem();
void _Exec_AllocEntry();
void _Exec_FreeEntry();
void _Exec_Insert();
void _Exec_AddHead();
void _Exec_AddTail();
void _Exec_Remove();
void _Exec_RemHead();
void _Exec_RemTail();
void _Exec_Enqueue();
void _Exec_FindName();
void _Exec_AddTask();
void _Exec_RemTask();
void _Exec_FindTask();
void _Exec_SetTaskPri();
void _Exec_SetSignal();
void _Exec_SetExcept();
void _Exec_Wait();
void _Exec_Signal();
void _Exec_AllocSignal();
void _Exec_FreeSignal();
void _Exec_AddPort();
void _Exec_RemPort();
void _Exec_PutMsg();
void _Exec_GetMsg();
void _Exec_ReplyMsg();
void _Exec_WaitPort();
void _Exec_FindPort();
void _Exec_AddLibrary();
void _Exec_RemLibrary();
void _Exec_OldOpenLibrary();
void _Exec_CloseLibrary();
void _Exec_SetFunction();
void _Exec_SumLibrary();
void _Exec_AddDevice();
void _Exec_RemDevice();
void _Exec_OpenDevice();
void _Exec_CloseDevice();
void _Exec_DoIO();
void _Exec_SendIO();
void _Exec_CheckIO();
void _Exec_WaitIO();
void _Exec_AbortIO();
void _Exec_AddResource();
void _Exec_RemResource();
void _Exec_OpenResource();
void _Exec_RawDoFmt();
void _Exec_TypeOfMem();
void _Exec_Procure();
void _Exec_Vacate();
void _Exec_OpenLibrary();
void _Exec_InitSemaphore();
void _Exec__ObtainSemaphore();
void _Exec__ReleaseSemaphore();
void _Exec_AttemptSemaphore();
void _Exec_ObtainSemaphoreList();
void _Exec_ReleaseSemaphoreList();
void _Exec_FindSemaphore();
void _Exec_AddSemaphore();
void _Exec_RemSemaphore();
void _Exec_AddMemList();
void _Exec_CopyMem();
void _Exec_CopyMemQuick();
void _Exec_CacheClearU();
void _Exec_CacheClearE();
void _Exec_CacheControl();
void _Exec_CreateIORequest();
void _Exec_DeleteIORequest();
void _Exec_CreateMsgPort();
void _Exec_DeleteMsgPort();
void _Exec__ObtainSemaphoreShared();
void _Exec_AllocVec();
void _Exec_FreeVec();
void _Exec_CreatePool();
void _Exec_DeletePool();
void _Exec_AllocPooled();
void _Exec_FreePooled();
void _Exec_AttemptSemaphoreShared();
void _Exec_StackSwap();
void _Exec_CachePreDMA();
void _Exec_CachePostDMA();
void _Exec_AddMemHandler();
void _Exec_RemMemHandler();

void *ExecFunctions[131]=
{
/*  1 */NULL,
	NULL,
	NULL,
	NULL,
	&_Exec_Supervisor,
	&_Exec_Switch,
	&_Exec_Dispatch,
	&_Exec_Exception,
	&_Exec_PrepareContext,
/* 10 */NULL,		/* Private5 */
	NULL,		/* Private6 */
	NULL,		/* InitCode */
	&_Exec_InitStruct,
	&_Exec_MakeLibrary,
	&_Exec_MakeFunctions,
	NULL,		/* FindResident */
	&_Exec_InitResident,
	&_Exec_Alert,
	NULL,		/* Debug */
/* 20 */&_Exec_Disable,
	&_Exec_Enable,
	&_Exec_Forbid,
	&_Exec_Permit,
	&_Exec_SetSR,
	&_Exec_SuperState,
	&_Exec_UserState,
	NULL,		/* SetIntVector */
	NULL,		/* AddIntServer */
	NULL,		/* RemIntServer */
/* 30 */NULL,		/* Cause */
	&_Exec_Allocate,
	&_Exec_Deallocate,
	&_Exec_AllocMem,
	&_Exec_AllocAbs,
	&_Exec_FreeMem,
	&_Exec_AvailMem,
	&_Exec_AllocEntry,
	&_Exec_FreeEntry,
	&_Exec_Insert,
/* 40 */&_Exec_AddHead,
	&_Exec_AddTail,
	&_Exec_Remove,
	&_Exec_RemHead,
	&_Exec_RemTail,
	&_Exec_Enqueue,
	&_Exec_FindName,
	&_Exec_AddTask,
	&_Exec_RemTask,
	&_Exec_FindTask,
/* 50 */&_Exec_SetTaskPri,
	&_Exec_SetSignal,
	&_Exec_SetExcept,
	&_Exec_Wait,
	&_Exec_Signal,
	&_Exec_AllocSignal,
	&_Exec_FreeSignal,
	NULL,		/* AllocTrap */
	NULL,		/* FreeTrap */
	&_Exec_AddPort,
/* 60 */&_Exec_RemPort,
	&_Exec_PutMsg,
	&_Exec_GetMsg,
	&_Exec_ReplyMsg,
	&_Exec_WaitPort,
	&_Exec_FindPort,
	&_Exec_AddLibrary,
	&_Exec_RemLibrary,
	&_Exec_OldOpenLibrary,
	&_Exec_CloseLibrary,
/* 70 */&_Exec_SetFunction,
	&_Exec_SumLibrary,
	&_Exec_AddDevice,
	&_Exec_RemDevice,
	&_Exec_OpenDevice,
	&_Exec_CloseDevice,
	&_Exec_DoIO,
	&_Exec_SendIO,
	&_Exec_CheckIO,
	&_Exec_WaitIO,
/* 80 */&_Exec_AbortIO,
	&_Exec_AddResource,
	&_Exec_RemResource,
	&_Exec_OpenResource,
	NULL,		/* Private7 */
	NULL,		/* Private8 */
	NULL,		/* Private9 */
	&_Exec_RawDoFmt,
	NULL,		/* GetCC */
	&_Exec_TypeOfMem,
/* 90 */&_Exec_Procure,
	&_Exec_Vacate,
	&_Exec_OpenLibrary,
	&_Exec_InitSemaphore,
	&_Exec__ObtainSemaphore,
	&_Exec__ReleaseSemaphore,
	&_Exec_AttemptSemaphore,
	&_Exec_ObtainSemaphoreList,
	&_Exec_ReleaseSemaphoreList,
	&_Exec_FindSemaphore,
/*100 */&_Exec_AddSemaphore,
	&_Exec_RemSemaphore,
	NULL,		/* SumKickData */
	&_Exec_AddMemList,
	&_Exec_CopyMem,
	&_Exec_CopyMemQuick,
	&_Exec_CacheClearU,
	&_Exec_CacheClearE,
	&_Exec_CacheControl,
	&_Exec_CreateIORequest,
/*110 */&_Exec_DeleteIORequest,
	&_Exec_CreateMsgPort,
	&_Exec_DeleteMsgPort,
	&_Exec__ObtainSemaphoreShared,
	&_Exec_AllocVec,
	&_Exec_FreeVec,
	&_Exec_CreatePool,
	&_Exec_DeletePool,
	&_Exec_AllocPooled,
	&_Exec_FreePooled,
/*120 */&_Exec_AttemptSemaphoreShared,
	NULL,		/* ColdReboot */
	&_Exec_StackSwap,
	NULL,		/* ChildFree */
	NULL,		/* ChildOrphan */
	NULL,		/* ChildStatus */
	NULL,		/* ChildWait */
	&_Exec_CachePreDMA,
	&_Exec_CachePostDMA,
	&_Exec_AddMemHandler,
/*130 */&_Exec_RemMemHandler,
	NULL		/* ObtainQuickVector */
};
