#ifndef _EXEC_UTIL_H
#define _EXEC_UTIL_H

/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Utility functions for exec.
    Lang: english
*/

#include <aros/asmcall.h>
#include <exec/types.h>
#include <stdarg.h>

#ifndef EXEC_TASKS_H
struct Task;
#endif
#ifndef EXEC_LISTS_H
struct List;
#endif
#ifndef ETASK_H
struct IntETask;
#endif

/*
    Prototypes
*/
APTR Exec_AllocTaskMem (struct Task * task, ULONG size, ULONG flags, struct ExecBase *SysBase);
void Exec_FreeTaskMem (struct Task * task, APTR mem, struct ExecBase *SysBase);

void Exec_InitETask(struct Task *task, struct ETask *etask, struct ExecBase *SysBase);
void Exec_CleanupETask(struct Task *task, struct ETask *et, struct ExecBase *SysBase);
struct Task *Exec_FindTaskByID(ULONG id, struct ExecBase *SysBase);
struct ETask *Exec_FindChild(ULONG id, struct ExecBase *SysBase);
struct IntETask *FindETask(struct List *, ULONG id, struct ExecBase *SysBase);

STRPTR Alert_AddString(STRPTR dest, STRPTR src);
STRPTR Alert_GetTitle(ULONG alertNum);
STRPTR Alert_GetTaskName(struct Task *task);
void Alert_GetString(ULONG alertnum, STRPTR buf);
VOID Exec_CrashHandler(void);
ULONG Exec_UserAlert(ULONG alertNum, struct Task *task);

APTR InternalRawDoFmt(CONST_STRPTR FormatString, APTR DataStream, VOID_FUNC PutChProc,
		      APTR PutChData, va_list VaListStream);

/*
 *  Pseudo-functions, including SysBase for nicer calling...
 */
#define AllocTaskMem(t,s,f) Exec_AllocTaskMem(t,s,f,SysBase)
#define FreeTaskMem(t,m)    Exec_FreeTaskMem(t,m,SysBase)
#define FindTaskByID(i)	    Exec_FindTaskByID(i,SysBase)
#define FindChild(i)	    Exec_FindChild(i,SysBase)
#define FindETask(l,i)	    Exec_FindETask(l,i,SysBase)
#define InitETask(t,e)	    Exec_InitETask(t,e,SysBase)
#define CleanupETask(t,e)   Exec_CleanupETask(t,e,SysBase)

#endif /* _EXEC_UTIL_H */
