#ifndef _EXEC_UTIL_H
#define _EXEC_UTIL_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Utility functions for exec.
    Lang: english
*/

#include <aros/asmcall.h>
#include <exec/types.h>
#include <utility/tagitem.h>
#include <proto/alib.h>
#include <proto/arossupport.h>

#include <stdarg.h>

#include "exec_intern.h"

/* PC and FP registers for various CPUs */
#ifdef __i386__
#define PC eip
#define FP ebp
#endif
#ifdef __x86_64__
#define PC rip
#define FP rbp
#endif
#ifdef __mc68000__
#define PC pc
#ifdef CONFIG_GCC_FP_A6
#define FP a[6]
#else
#define FP a[5]
#define CALLER_FRAME NULL
#endif
#endif
#ifdef __powerpc__
#define PC ip
#define FP gpr[1]
#endif
#ifdef __arm__
#define PC pc
#define FP r[11]
/* __builtin_frame_address(1) returns LR value. Perhaps not AAPCS-compatible. */
#define CALLER_FRAME ({void * _fp; asm volatile("ldr %0, [%%fp, #-4]":"=r"(_fp)); _fp;})
#endif

#ifndef PC
#error unsupported CPU type
#endif

#ifndef CALLER_FRAME
#define CALLER_FRAME __builtin_frame_address(1)
#endif

#ifndef EXEC_TASKS_H
struct Task;
#endif
#ifndef EXEC_LISTS_H
struct List;
#endif
#ifndef ETASK_H
struct IntETask;
#endif

struct TraceLocation
{
    const char *function;
    APTR	caller;
    APTR	stack;
};

#define CURRENT_LOCATION(name) {name, __builtin_return_address(0), CALLER_FRAME}

/*
    Prototypes
*/
BOOL PrepareContext(struct Task *task, APTR entryPoint, APTR fallBack,
                    const struct TagItem *tagList, struct ExecBase *SysBase);

void Exec_InitETask(struct Task *task, struct ExecBase *SysBase);
void Exec_CleanupETask(struct Task *task, struct ExecBase *SysBase);
void Exec_ExpungeETask(struct ETask *et, struct ExecBase *SysBase);
BOOL Exec_ExpandTS(struct Task *task, struct ExecBase *SysBase);
struct ETask *Exec_FindChild(ULONG id, struct ExecBase *SysBase);
struct IntETask *FindETask(struct List *, ULONG id, struct ExecBase *SysBase);

BOOL Exec_CheckTask(struct Task *task, struct ExecBase *SysBase);

STRPTR Alert_AddString(STRPTR dest, CONST_STRPTR src);
STRPTR Alert_GetTitle(ULONG alertNum);
STRPTR Alert_GetString(ULONG alertnum, STRPTR buf);
STRPTR FormatAlert(char *buffer, ULONG alertNum, struct Task *task, APTR location, UBYTE type, struct ExecBase *SysBase);
STRPTR FormatTask(STRPTR buffer, const char *text, struct Task *, struct ExecBase *SysBase);
STRPTR FormatLocation(STRPTR buf, const char *text, APTR location, struct ExecBase *SysBase);

void FormatAlertExtra(char *buffer, APTR stack, UBYTE type, APTR data, struct ExecBase *SysBase);
char *FormatCPUContext(char *buffer, struct ExceptionContext *ctx, struct ExecBase *SysBase);
APTR UnwindFrame(APTR fp, APTR *caller);

void Exec_ExtAlert(ULONG alertNum, APTR location, APTR stack, UBYTE type, APTR data, struct ExecBase *SysBase);
ULONG Exec_UserAlert(ULONG alertNum, struct ExecBase *SysBase);
void Exec_SystemAlert(ULONG alertNum, APTR location, APTR stack, UBYTE type, APTR data, struct ExecBase *SysBase);
void Exec_DoResetCallbacks(struct IntExecBase *SysBase);

APTR InternalRawDoFmt(CONST_STRPTR FormatString, APTR DataStream, VOID_FUNC PutChProc,
		      APTR PutChData, va_list VaListStream);

IPTR *InternalFindResident(const UBYTE *name, IPTR *list);

void FastPutMsg(struct MsgPort *port, struct Message *message, struct ExecBase *SysBase);
void InternalPutMsg(struct MsgPort *port, struct Message *message, struct ExecBase *SysBase);

LONG AllocTaskSignal(struct Task *ThisTask, LONG signalNum, struct ExecBase *SysBase);

static inline void InitMsgPort(struct MsgPort *ret)
{
    /* Set port to type 'signalling' */
    ret->mp_Flags = PA_SIGNAL;
    /* Set port to type MsgPort */
    ret->mp_Node.ln_Type = NT_MSGPORT;
    /* Clear the list of messages */
    NEWLIST(&ret->mp_MsgList);
}

/* Pseudo-functions, including SysBase for nicer calling */
#define FindChild(i)	    Exec_FindChild(i,SysBase)
#define FindETask(l,i)	    Exec_FindETask(l,i,SysBase)
#define InitETask(t)	    Exec_InitETask(t,SysBase)
#define CleanupETask(t)     Exec_CleanupETask(t,SysBase)
#define ExpungeETask(e)	    Exec_ExpungeETask(e,SysBase)

#endif /* _EXEC_UTIL_H */
