/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
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

void Exec_Supervisor();
void Exec_Switch();
void Exec_Dispatch();
void Exec_Exception();
void Exec_PrepareContext();
void Exec_InitStruct();
void Exec_MakeLibrary();
void Exec_MakeFunctions();
void Exec_InitResident();
void Exec_Alert();
void Exec_Disable();
void Exec_Enable();
void Exec_Forbid();
void Exec_Permit();
void Exec_SetSR();
void Exec_SuperState();
void Exec_UserState();
void Exec_Allocate();
void Exec_Deallocate();
void Exec_AllocMem();
void Exec_AllocAbs();
void Exec_FreeMem();
void Exec_AvailMem();
void Exec_AllocEntry();
void Exec_FreeEntry();
void Exec_Insert();
void Exec_AddHead();
void Exec_AddTail();
void Exec_Remove();
void Exec_RemHead();
void Exec_RemTail();
void Exec_Enqueue();
void Exec_FindName();
void Exec_AddTask();
void Exec_RemTask();
void Exec_FindTask();
void Exec_SetTaskPri();
void Exec_SetSignal();
void Exec_SetExcept();
void Exec_Wait();
void Exec_Signal();
void Exec_AllocSignal();
void Exec_FreeSignal();
void Exec_AddPort();
void Exec_RemPort();
void Exec_PutMsg();
void Exec_GetMsg();
void Exec_ReplyMsg();
void Exec_WaitPort();
void Exec_FindPort();
void Exec_AddLibrary();
void Exec_RemLibrary();
void Exec_OldOpenLibrary();
void Exec_CloseLibrary();
void Exec_SetFunction();
void Exec_SumLibrary();
void Exec_AddDevice();
void Exec_RemDevice();
void Exec_OpenDevice();
void Exec_CloseDevice();
void Exec_DoIO();
void Exec_SendIO();
void Exec_CheckIO();
void Exec_WaitIO();
void Exec_AbortIO();
void Exec_AddResource();
void Exec_RemResource();
void Exec_OpenResource();
void Exec_RawDoFmt();
void Exec_TypeOfMem();
void Exec_Procure();
void Exec_Vacate();
void Exec_OpenLibrary();
void Exec_InitSemaphore();
void Exec__ObtainSemaphore();
void Exec__ReleaseSemaphore();
void Exec_AttemptSemaphore();
void Exec_ObtainSemaphoreList();
void Exec_ReleaseSemaphoreList();
void Exec_FindSemaphore();
void Exec_AddSemaphore();
void Exec_RemSemaphore();
void Exec_AddMemList();
void Exec_CopyMem();
void Exec_CopyMemQuick();
void Exec_CacheClearU();
void Exec_CacheClearE();
void Exec_CacheControl();
void Exec_CreateIORequest();
void Exec_DeleteIORequest();
void Exec_CreateMsgPort();
void Exec_DeleteMsgPort();
void Exec__ObtainSemaphoreShared();
void Exec_AllocVec();
void Exec_FreeVec();
void Exec_CreatePool();
void Exec_DeletePool();
void Exec_AllocPooled();
void Exec_FreePooled();
void Exec_AttemptSemaphoreShared();
void Exec_StackSwap();
void Exec_CachePreDMA();
void Exec_CachePostDMA();
void Exec_AddMemHandler();
void Exec_RemMemHandler();

void *ExecFunctions[131]=
{
/*  1 */NULL,
	NULL,
	NULL,
	NULL,
	&Exec_Supervisor,
	&Exec_Switch,
	&Exec_Dispatch,
	&Exec_Exception,
	&Exec_PrepareContext,
/* 10 */NULL,		/* Private5 */
	NULL,		/* Private6 */
	NULL,		/* InitCode */
	&Exec_InitStruct,
	&Exec_MakeLibrary,
	&Exec_MakeFunctions,
	NULL,		/* FindResident */
	&Exec_InitResident,
	&Exec_Alert,
	NULL,		/* Debug */
/* 20 */&Exec_Disable,
	&Exec_Enable,
	&Exec_Forbid,
	&Exec_Permit,
	&Exec_SetSR,
	&Exec_SuperState,
	&Exec_UserState,
	NULL,		/* SetIntVector */
	NULL,		/* AddIntServer */
	NULL,		/* RemIntServer */
/* 30 */NULL,		/* Cause */
	&Exec_Allocate,
	&Exec_Deallocate,
	&Exec_AllocMem,
	&Exec_AllocAbs,
	&Exec_FreeMem,
	&Exec_AvailMem,
	&Exec_AllocEntry,
	&Exec_FreeEntry,
	&Exec_Insert,
/* 40 */&Exec_AddHead,
	&Exec_AddTail,
	&Exec_Remove,
	&Exec_RemHead,
	&Exec_RemTail,
	&Exec_Enqueue,
	&Exec_FindName,
	&Exec_AddTask,
	&Exec_RemTask,
	&Exec_FindTask,
/* 50 */&Exec_SetTaskPri,
	&Exec_SetSignal,
	&Exec_SetExcept,
	&Exec_Wait,
	&Exec_Signal,
	&Exec_AllocSignal,
	&Exec_FreeSignal,
	NULL,		/* AllocTrap */
	NULL,		/* FreeTrap */
	&Exec_AddPort,
/* 60 */&Exec_RemPort,
	&Exec_PutMsg,
	&Exec_GetMsg,
	&Exec_ReplyMsg,
	&Exec_WaitPort,
	&Exec_FindPort,
	&Exec_AddLibrary,
	&Exec_RemLibrary,
	&Exec_OldOpenLibrary,
	&Exec_CloseLibrary,
/* 70 */&Exec_SetFunction,
	&Exec_SumLibrary,
	&Exec_AddDevice,
	&Exec_RemDevice,
	&Exec_OpenDevice,
	&Exec_CloseDevice,
	&Exec_DoIO,
	&Exec_SendIO,
	&Exec_CheckIO,
	&Exec_WaitIO,
/* 80 */&Exec_AbortIO,
	&Exec_AddResource,
	&Exec_RemResource,
	&Exec_OpenResource,
	NULL,		/* Private7 */
	NULL,		/* Private8 */
	NULL,		/* Private9 */
	&Exec_RawDoFmt,
	NULL,		/* GetCC */
	&Exec_TypeOfMem,
/* 90 */&Exec_Procure,
	&Exec_Vacate,
	&Exec_OpenLibrary,
	&Exec_InitSemaphore,
	&Exec__ObtainSemaphore,
	&Exec__ReleaseSemaphore,
	&Exec_AttemptSemaphore,
	&Exec_ObtainSemaphoreList,
	&Exec_ReleaseSemaphoreList,
	&Exec_FindSemaphore,
/*100 */&Exec_AddSemaphore,
	&Exec_RemSemaphore,
	NULL,		/* SumKickData */
	&Exec_AddMemList,
	&Exec_CopyMem,
	&Exec_CopyMemQuick,
	&Exec_CacheClearU,
	&Exec_CacheClearE,
	&Exec_CacheControl,
	&Exec_CreateIORequest,
/*110 */&Exec_DeleteIORequest,
	&Exec_CreateMsgPort,
	&Exec_DeleteMsgPort,
	&Exec__ObtainSemaphoreShared,
	&Exec_AllocVec,
	&Exec_FreeVec,
	&Exec_CreatePool,
	&Exec_DeletePool,
	&Exec_AllocPooled,
	&Exec_FreePooled,
/*120 */&Exec_AttemptSemaphoreShared,
	NULL,		/* ColdReboot */
	&Exec_StackSwap,
	NULL,		/* ChildFree */
	NULL,		/* ChildOrphan */
	NULL,		/* ChildStatus */
	NULL,		/* ChildWait */
	&Exec_CachePreDMA,
	&Exec_CachePostDMA,
	&Exec_AddMemHandler,
/*130 */&Exec_RemMemHandler,
	NULL		/* ObtainQuickVector */
};
