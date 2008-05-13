/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/alerts.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <dos/dosextens.h>

#include <stddef.h>

#define DEFINE(sym, val) \
    asm volatile("\n#define " #sym " %0 ": : "i" (val))

#define FuncOffset(x)       (int)__AROS_GETJUMPVEC(0,x)

int main(void) {
    asm volatile("\n/* Macros */" ::);

    asm volatile("\n#define AROS_CSYMNAME(n)       n\n" ::);
    asm volatile("\n#define AROS_CDEFNAME(n)       n\n" ::);
    asm volatile("\n#define AROS_SLIB_ENTRY(n,s)   s ## _ ## n\n" ::);
    
    asm volatile("\n#define _FUNCTION(n)           .type   n,@function" ::);
    asm volatile("\n#define _ALIGNMENT             .balign %0" :: "i" (AROS_WORSTALIGN));

    asm volatile("\n/* ExecBase */" ::);
    DEFINE(AttnResched   , offsetof (struct ExecBase, AttnResched));
    DEFINE(IDNestCnt     , offsetof (struct ExecBase, IDNestCnt));
    DEFINE(TDNestCnt     , offsetof (struct ExecBase, TDNestCnt));
    DEFINE(TaskReady     , offsetof (struct ExecBase, TaskReady));
    DEFINE(ThisTask      , offsetof (struct ExecBase, ThisTask));
    DEFINE(SysFlags      , offsetof (struct ExecBase, SysFlags));
    DEFINE(IdleCount     , offsetof (struct ExecBase, IdleCount));
    DEFINE(DispCount     , offsetof (struct ExecBase, DispCount));
    DEFINE(Quantum       , offsetof (struct ExecBase, Quantum));
    DEFINE(Elapsed       , offsetof (struct ExecBase, Elapsed));

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
    DEFINE(iet_Context   , sizeof (struct ETask) + 4);

    asm volatile("\n/* struct DosBase */" ::);
    DEFINE(dl_SysBase    , offsetof (struct DosLibrary, dl_SysBase));

    asm volatile("\n/* struct StackSwapStruct */" ::);
    DEFINE(stk_Lower     , offsetof (struct StackSwapStruct, stk_Lower));
    DEFINE(stk_Upper     , offsetof (struct StackSwapStruct, stk_Upper));
    DEFINE(stk_Pointer   , offsetof (struct StackSwapStruct, stk_Pointer));

    asm volatile("\n/* Task Flags */" ::);
    DEFINE(TS_RUN        , TS_RUN);
    DEFINE(TS_READY      , TS_READY);
    DEFINE(TF_EXCEPT     , TF_EXCEPT);
    DEFINE(TF_SWITCH     , TF_SWITCH);
    DEFINE(TF_LAUNCH     , TF_LAUNCH);

    asm volatile("\n/* Exec functions */" ::);
    DEFINE(Reschedule    , FuncOffset (8));
    DEFINE(Switch        , FuncOffset (9));
    DEFINE(Dispatch      , FuncOffset (10));
    DEFINE(Exception     , FuncOffset (11));
    DEFINE(Alert         , FuncOffset (18));
    DEFINE(Disable       , FuncOffset (20));
    DEFINE(Enable        , FuncOffset (21));
    DEFINE(Enqueue       , FuncOffset (45));
    DEFINE(FindTask      , FuncOffset (49));
    DEFINE(StackSwap     , FuncOffset (122));

    asm volatile("\n/* Constants */" ::);
    DEFINE(AT_DeadEnd    , AT_DeadEnd);
    DEFINE(AN_StackProbe , AN_StackProbe);

    DEFINE(ln_Succ       , offsetof (struct Node, ln_Succ));
    DEFINE(ln_Pred       , offsetof (struct Node, ln_Pred));
    DEFINE(ln_Pri        , offsetof (struct Node, ln_Pri));

    DEFINE(lh_Head       , offsetof (struct List, lh_Head));
    DEFINE(lh_TailPred   , offsetof (struct List, lh_TailPred));


#ifdef UseExecstubs
    asm volatile("\n#define UseExecstubs 1" ::);
#endif

    return 0;
}
