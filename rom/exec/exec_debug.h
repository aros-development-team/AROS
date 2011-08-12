#ifndef _EXEC_DEBUG_H_
#define _EXEC_DEBUG_H_ 1
/* Debug which function of Exec */

#include <exec/execbase.h>

#define DEBUG_AbortIO 0
#define DEBUG_AddDevice 0
#define DEBUG_AddHead 0
#define DEBUG_AddLibrary 0
#define DEBUG_AddMemHandler 0
#define DEBUG_AddMemList 0
#define DEBUG_AddPort 0
#define DEBUG_AddResource 0
#define DEBUG_AddSemaphore 0
#define DEBUG_AddTail 0
#define DEBUG_Alert 0
#define DEBUG_AllocAbs 0
#define DEBUG_AllocEntry 0
#define DEBUG_AllocMem 0
#define DEBUG_AllocPooled 0
#define DEBUG_AllocSignal 0
#define DEBUG_AllocVec 0
#define DEBUG_Allocate 0
#define DEBUG_AttemptSemaphore 0
#define DEBUG_AttemptSemaphoreShared 0
#define DEBUG_AvailMem 0
#define DEBUG_CacheClearE 0
#define DEBUG_Cause 0
#define DEBUG_CheckIO 0
#define DEBUG_CloseDevice 0
#define DEBUG_CloseLibrary 0
#define DEBUG_CopyMem 0
#define DEBUG_CopyMemQuick 0
#define DEBUG_CreateIORequest 0
#define DEBUG_CreatePool 0
#define DEBUG_Deallocate 0
#define DEBUG_DeleteIORequest 0
#define DEBUG_DeleteMsgPort 0
#define DEBUG_DeletePool 0
#define DEBUG_DoIO 0
#define DEBUG_Enqueue 0
#define DEBUG_FindName 0
#define DEBUG_FindPort 0
#define DEBUG_FindSemaphore 0
#define DEBUG_FindTask 0
#define DEBUG_FreeEntry 0
#define DEBUG_FreeMem 0
#define DEBUG_FreePooled 0
#define DEBUG_FreeSignal 0
#define DEBUG_FreeVec 0
#define DEBUG_GetMsg 0
#define DEBUG_InitSemaphore 0
#define DEBUG_InitStruct 0
#define DEBUG_Insert 0
#define DEBUG_NewAllocEntry 0
#define DEBUG_ObtainQuickVector 0
#define DEBUG_ObtainSemaphore 0
#define DEBUG_ObtainSemaphoreList 0
#define DEBUG_ObtainSemaphoreShared 0
#define DEBUG_OldOpenLibrary 0
#define DEBUG_OpenDevice 0
#define DEBUG_OpenResource 0
#define DEBUG_PrepareContext 0
#define DEBUG_Procure 0
#define DEBUG_PutMsg 0
#define DEBUG_RawDoFmt 0
#define DEBUG_ReleaseSemaphore 0
#define DEBUG_ReleaseSemaphoreList 0
#define DEBUG_RemDevice 0
#define DEBUG_RemHead 0
#define DEBUG_RemLibrary 0
#define DEBUG_RemMemHandler 0
#define DEBUG_RemPort 0
#define DEBUG_RemResource 0
#define DEBUG_RemSemaphore 0
#define DEBUG_RemTail 0
#define DEBUG_Remove 0
#define DEBUG_ReplyMsg 0
#define DEBUG_SendIO 0
#define DEBUG_SetExcept 0
#define DEBUG_SetFunction 0
#define DEBUG_SetSignal 0
#define DEBUG_SetTaskPri 0
#define DEBUG_Signal 0
#define DEBUG_SumLibrary 0
#define DEBUG_TypeOfMem 0
#define DEBUG_Vacate 0
#define DEBUG_Wait 0
#define DEBUG_WaitIO 0
#define DEBUG_WaitPort 0

/* Runtime debugging */
#ifdef NO_RUNTIME_DEBUG

#define ExecLog(...) do { } while (0)
#define ParseFlags(...) (0)

#else

#include <stdarg.h>	/* for va_list */

extern const char * const ExecFlagNames[];
ULONG ParseFlags(char *opts, const char * const *FlagNames);
void ExecLog(struct ExecBase *SysBase, ULONG flags, const char *format, ...);
void VLog(struct ExecBase *SysBase, ULONG flags, const char * const *FlagNames, const char *format, va_list args);

#endif

#define DINITCODE(...)		ExecLog(SysBase, EXECDEBUGF_INITCODE, __VA_ARGS__)
#define DINITRESIDENT(...) 	ExecLog(SysBase, EXECDEBUGF_INITRESIDENT, __VA_ARGS__)
#define DFINDRESIDENT(...)	ExecLog(SysBase, EXECDEBUGF_FINDRESIDENT, __VA_ARGS__)
#define DCREATELIBRARY(...)	ExecLog(SysBase, EXECDEBUGF_CREATELIBRARY, __VA_ARGS__)
#define DSETFUNCTION(...)	ExecLog(SysBase, EXECDEBUGF_SETFUNCTION, __VA_ARGS__)
#define DADDTASK(...)		ExecLog(SysBase, EXECDEBUGF_ADDTASK, __VA_ARGS__)
#define DREMTASK(...)		ExecLog(SysBase, EXECDEBUGF_REMTASK, __VA_ARGS__)
#define DINIT(...)		ExecLog(SysBase, EXECDEBUGF_INIT, __VA_ARGS__)
#define DRAMLIB(...)		ExecLog(SysBase, EXECDEBUGF_RAMLIB, __VA_ARGS__)

#endif /* _EXEC_DEBUG_H_ */
