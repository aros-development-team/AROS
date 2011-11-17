/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/alerts.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/tasks.h>
#include <dos/dosextens.h>
#include <graphics/clip.h>

#include <setjmp.h>
#include <stddef.h>

#ifdef __mc68000
#include "exec_intern.h"
#undef KernelBase
#include "kernel_base.h"
#include "kernel_intern.h"

    /* m68k relative addresses must *not* start with a '#' */
#define DEFINE(sym, val) \
    asm volatile("\n#define " #sym " %c0 ": : "i" (val))
#else
#define DEFINE(sym, val) \
    asm volatile("\n#define " #sym " %0 ": : "i" (val))
#endif

#define FuncOffset(x)       (long)__AROS_GETJUMPVEC(0,x)

int main(void) {
    asm volatile("\n/* Macros */" ::);

    asm volatile("\n#define AROS_CSYMNAME(n)       n\n" ::);
    asm volatile("\n#define AROS_CDEFNAME(n)       n\n" ::);
    asm volatile("\n#define AROS_SLIB_ENTRY(n,s,o)   s ## _ ## o ## _ ## n\n" ::);
    
    asm volatile("\n#define _FUNCTION(n)           .type   n,@function" ::);
    asm volatile("\n#define _ALIGNMENT             .balign %0" :: "i" (AROS_WORSTALIGN));

    asm volatile("\n/* ExecBase */" ::);
    DEFINE(AttnResched   , offsetof (struct ExecBase, AttnResched));
    DEFINE(AttnFlags     , offsetof (struct ExecBase, AttnFlags));
    DEFINE(IDNestCnt     , offsetof (struct ExecBase, IDNestCnt));
    DEFINE(TDNestCnt     , offsetof (struct ExecBase, TDNestCnt));
    DEFINE(TaskReady     , offsetof (struct ExecBase, TaskReady));
    DEFINE(ThisTask      , offsetof (struct ExecBase, ThisTask));
    DEFINE(SysFlags      , offsetof (struct ExecBase, SysFlags));
    DEFINE(IdleCount     , offsetof (struct ExecBase, IdleCount));
    DEFINE(DispCount     , offsetof (struct ExecBase, DispCount));
    DEFINE(Quantum       , offsetof (struct ExecBase, Quantum));
    DEFINE(Elapsed       , offsetof (struct ExecBase, Elapsed));
    DEFINE(SysStkUpper   , offsetof (struct ExecBase, SysStkUpper));

    asm volatile("\n/* struct Task */" ::);
    DEFINE(tc_State      , offsetof (struct Task, tc_State));
    DEFINE(tc_Flags      , offsetof (struct Task, tc_Flags));
    DEFINE(tc_ExceptCode , offsetof (struct Task, tc_ExceptCode));
    DEFINE(tc_ExceptData , offsetof (struct Task, tc_ExceptData));
    DEFINE(tc_SigExcept  , offsetof (struct Task, tc_SigExcept));
    DEFINE(tc_SigRecvd   , offsetof (struct Task, tc_SigRecvd));
    DEFINE(tc_Launch     , offsetof (struct Task, tc_Launch));
    DEFINE(tc_Switch     , offsetof (struct Task, tc_Switch));
    DEFINE(tc_SPReg      , offsetof (struct Task, tc_SPReg));
    DEFINE(tc_SPLower    , offsetof (struct Task, tc_SPLower));
    DEFINE(tc_SPUpper    , offsetof (struct Task, tc_SPUpper));
    DEFINE(tc_IDNestCnt  , offsetof (struct Task, tc_IDNestCnt));
    DEFINE(tc_ETask      , offsetof (struct Task, tc_UnionETask.tc_ETask));
    DEFINE(tc_TaskStorage, offsetof (struct Task, tc_UnionETask.tc_TaskStorage));

    asm volatile("\n/* struct Process */" ::);
    DEFINE(pr_CES        , offsetof (struct Process, pr_CES));
    DEFINE(pr_CIS        , offsetof (struct Process, pr_CIS));
    DEFINE(pr_CLI        , offsetof (struct Process, pr_CLI));
    DEFINE(pr_COS        , offsetof (struct Process, pr_COS));
    DEFINE(pr_CurrentDir , offsetof (struct Process, pr_CurrentDir));
    DEFINE(pr_ConsoleTask, offsetof (struct Process, pr_ConsoleTask));
    DEFINE(pr_FileSystemTask, offsetof (struct Process, pr_FileSystemTask));
    DEFINE(pr_MsgPort    , offsetof (struct Process, pr_MsgPort));
    DEFINE(pr_Result2    , offsetof (struct Process, pr_Result2));
    DEFINE(pr_ReturnAddr , offsetof (struct Process, pr_ReturnAddr));
    DEFINE(pr_SegList    , offsetof (struct Process, pr_SegList));
    DEFINE(pr_WindowPtr  , offsetof (struct Process, pr_WindowPtr));

    asm volatile("\n/* struct DosBase */" ::);
    DEFINE(dl_Root         , offsetof (struct DosLibrary, dl_Root));
    DEFINE(dl_TimeReq      , offsetof (struct DosLibrary, dl_TimeReq));
    DEFINE(dl_UtilityBase  , offsetof (struct DosLibrary, dl_UtilityBase));
    DEFINE(dl_IntuitionBase, offsetof (struct DosLibrary, dl_IntuitionBase));

    asm volatile("\n/* struct DosPacket */" ::);
    DEFINE(dp_Link       , offsetof (struct DosPacket, dp_Link));
    DEFINE(dp_Port       , offsetof (struct DosPacket, dp_Port));
    DEFINE(dp_Type       , offsetof (struct DosPacket, dp_Type));
    DEFINE(dp_Res1       , offsetof (struct DosPacket, dp_Res1));
    DEFINE(dp_Res2       , offsetof (struct DosPacket, dp_Res2));
    DEFINE(dp_Arg1       , offsetof (struct DosPacket, dp_Arg1));
    DEFINE(dp_Arg2       , offsetof (struct DosPacket, dp_Arg2));
    DEFINE(dp_Arg3       , offsetof (struct DosPacket, dp_Arg3));
    DEFINE(dp_Arg4       , offsetof (struct DosPacket, dp_Arg4));
    DEFINE(dp_Arg5       , offsetof (struct DosPacket, dp_Arg5));
    DEFINE(dp_Arg6       , offsetof (struct DosPacket, dp_Arg6));
    DEFINE(dp_Arg7       , offsetof (struct DosPacket, dp_Arg7));

    asm volatile("\n/* struct FileHandle */" ::);
    DEFINE(fh_Flags      , offsetof (struct FileHandle, fh_Flags));
    DEFINE(fh_Interactive, offsetof (struct FileHandle, fh_Interactive));
    DEFINE(fh_Type       , offsetof (struct FileHandle, fh_Type ));
    DEFINE(fh_Buf        , offsetof (struct FileHandle, fh_Buf  ));
    DEFINE(fh_Pos        , offsetof (struct FileHandle, fh_Pos  ));
    DEFINE(fh_End        , offsetof (struct FileHandle, fh_End  ));

    asm volatile("\n/* struct IORequest */" ::);
    DEFINE(io_Message    , offsetof (struct IORequest, io_Message));
    DEFINE(io_Device     , offsetof (struct IORequest, io_Device ));
    DEFINE(io_Unit       , offsetof (struct IORequest, io_Unit   ));
    DEFINE(io_Command    , offsetof (struct IORequest, io_Command));
    DEFINE(io_Flags      , offsetof (struct IORequest, io_Flags  ));
    DEFINE(io_Error      , offsetof (struct IORequest, io_Error  ));

    asm volatile("\n/* struct timerequest */" ::);
    DEFINE(tr_time       , offsetof (struct timerequest, tr_time ));

    asm volatile("\n/* struct MsgPort */" ::);
    DEFINE(mp_SigTask    , offsetof (struct MsgPort, mp_SigTask));

    asm volatile("\n/* struct StackSwapStruct */" ::);
    DEFINE(stk_Lower     , offsetof (struct StackSwapStruct, stk_Lower));
    DEFINE(stk_Upper     , offsetof (struct StackSwapStruct, stk_Upper));
    DEFINE(stk_Pointer   , offsetof (struct StackSwapStruct, stk_Pointer));

    asm volatile("\n/* struct Layer */" ::);
    DEFINE(ly_Lock       , offsetof (struct Layer, Lock));

    asm volatile("\n/* Task Flags */" ::);
    DEFINE(TS_RUN        , TS_RUN);
    DEFINE(TS_READY      , TS_READY);
    DEFINE(TF_STACKCHK   , TF_STACKCHK);
    DEFINE(TF_EXCEPT     , TF_EXCEPT);
    DEFINE(TF_SWITCH     , TF_SWITCH);
    DEFINE(TF_LAUNCH     , TF_LAUNCH);

    asm volatile("\n/* Exec functions */" ::);
    DEFINE(Supervisor    , FuncOffset (5));
    DEFINE(Reschedule    , FuncOffset (8));
    DEFINE(Switch        , FuncOffset (9));
    DEFINE(Dispatch      , FuncOffset (10));
    DEFINE(Exception     , FuncOffset (11));
    DEFINE(Alert         , FuncOffset (18));
    DEFINE(Disable       , FuncOffset (20));
    DEFINE(Enable        , FuncOffset (21));
    DEFINE(Enqueue       , FuncOffset (45));
    DEFINE(FindTask      , FuncOffset (49));
    DEFINE(ObtainSemaphore,  FuncOffset (94));
    DEFINE(ReleaseSemaphore, FuncOffset (95));
    DEFINE(AttemptSemaphore, FuncOffset (96));
    DEFINE(StackSwap     , FuncOffset (122));

    asm volatile("\n/* Constants */" ::);
    DEFINE(AT_DeadEnd    , AT_DeadEnd);
    DEFINE(AN_StackProbe , AN_StackProbe);

    DEFINE(ln_Succ       , offsetof (struct Node, ln_Succ));
    DEFINE(ln_Pred       , offsetof (struct Node, ln_Pred));
    DEFINE(ln_Pri        , offsetof (struct Node, ln_Pri));
    DEFINE(ln_Name       , offsetof (struct Node, ln_Name));

    DEFINE(lh_Head       , offsetof (struct List, lh_Head));
    DEFINE(lh_TailPred   , offsetof (struct List, lh_TailPred));

    asm volatile("\n/* CPU context */" ::);
#ifdef __x86_64__
    DEFINE(ECF_SEGMENTS, ECF_SEGMENTS);

    DEFINE(Flags , offsetof(struct ExceptionContext, Flags));
    DEFINE(reg_ds, offsetof(struct ExceptionContext, ds));
    DEFINE(reg_es, offsetof(struct ExceptionContext, es));
    DEFINE(reg_fs, offsetof(struct ExceptionContext, fs));
    DEFINE(reg_gs, offsetof(struct ExceptionContext, gs));
#endif
#ifdef __i386__
    DEFINE(ECF_SEGMENTS, ECF_SEGMENTS);

    DEFINE(Flags , offsetof(struct ExceptionContext, Flags));
    DEFINE(reg_ds, offsetof(struct ExceptionContext, ds));
    DEFINE(reg_es, offsetof(struct ExceptionContext, es));
    DEFINE(reg_fs, offsetof(struct ExceptionContext, fs));
    DEFINE(reg_gs, offsetof(struct ExceptionContext, gs));
#endif
#ifdef __mc68000
    DEFINE(eb_KernelBase, offsetof(struct IntExecBase, KernelBase));
    DEFINE(kb_PlatformData, offsetof(struct KernelBase, kb_PlatformData));
    DEFINE(MMU_Level_A, offsetof(struct PlatformData, MMU_Level_A));
    DEFINE(zeropagedescriptor, offsetof(struct PlatformData, zeropagedescriptor));
    DEFINE(cachemodestore, offsetof(struct PlatformData, cachemodestore));
#endif

#ifdef UseExecstubs
    asm volatile("\n#define UseExecstubs 1" ::);
#endif

    asm volatile("\n/* jmp_buf */" ::);
    DEFINE(jmpbuf_SIZEOF, sizeof(jmp_buf));
    DEFINE(retaddr, offsetof(struct __jmp_buf, retaddr));

    return 0;
}
