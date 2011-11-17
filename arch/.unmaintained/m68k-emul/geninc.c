/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/types.h>
#include <stdio.h>
#include <stddef.h>
#include <exec/alerts.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <dos/dosextens.h>
#include <asm/sigcontext.h>
#include "machine.h"

#define FuncOffset(x)       (int)__AROS_GETJUMPVEC(0,x)

int main (void)
{
    printf ("/* Macros */\n"
	"#define AROS_CSYMNAME(n)       n\n"
	"#define AROS_CDEFNAME(n)       n\n"
	"#define AROS_SLIB_ENTRY(n,s)   s ## _ ## n\n"
	    "\n");

    printf ("/* ExecBase */\n");
    printf ("#define AttnResched   %d\n", offsetof (struct ExecBase, AttnResched));
    printf ("#define IDNestCnt     %d\n", offsetof (struct ExecBase, IDNestCnt));
    printf ("#define TDNestCnt     %d\n", offsetof (struct ExecBase, TDNestCnt));
    printf ("#define TaskReady     %d\n", offsetof (struct ExecBase, TaskReady));
    printf ("#define ThisTask      %d\n", offsetof (struct ExecBase, ThisTask));
    printf ("#define SysFlags      %d\n", offsetof (struct ExecBase, SysFlags));

    printf ("\n/* struct Task */\n");
    printf ("#define tc_State      %d\n", offsetof (struct Task, tc_State));
    printf ("#define tc_Flags      %d\n", offsetof (struct Task, tc_Flags));
    printf ("#define tc_ExceptCode %d\n", offsetof (struct Task, tc_ExceptCode));
    printf ("#define tc_ExceptData %d\n", offsetof (struct Task, tc_ExceptData));
    printf ("#define tc_SigExcept  %d\n", offsetof (struct Task, tc_SigExcept));
    printf ("#define tc_SigRecvd   %d\n", offsetof (struct Task, tc_SigRecvd));
    printf ("#define tc_Launch     %d\n", offsetof (struct Task, tc_Launch));
    printf ("#define tc_Switch     %d\n", offsetof (struct Task, tc_Switch));
    printf ("#define tc_SPReg      %d\n", offsetof (struct Task, tc_SPReg));
    printf ("#define tc_SPLower    %d\n", offsetof (struct Task, tc_SPLower));
    printf ("#define tc_SPUpper    %d\n", offsetof (struct Task, tc_SPUpper));
    printf ("#define tc_IDNestCnt  %d\n", offsetof (struct Task, tc_IDNestCnt));
    printf ("#define tc_UnionETask %d\n", offsetof (struct Task, tc_UnionETask));

    printf ("\n/* struct DosBase */\n");
    printf ("#define dl_SysBase    %d\n", offsetof (struct DosLibrary, dl_SysBase));

    printf ("\n/* struct StackSwapStruct */\n");
    printf ("#define stk_Lower     %d\n", offsetof (struct StackSwapStruct, stk_Lower));
    printf ("#define stk_Upper     %d\n", offsetof (struct StackSwapStruct, stk_Upper));
    printf ("#define stk_Pointer   %d\n", offsetof (struct StackSwapStruct, stk_Pointer));

    printf ("\n/* struct sigcontext_struct */\n");
    printf ("#define sc_mask       %d\n", offsetof (struct sigcontext_struct, sc_mask));
    printf ("#define sc_usp        %d\n", offsetof (struct sigcontext_struct, sc_usp));
    printf ("#define sc_d0         %d\n", offsetof (struct sigcontext_struct, sc_d0));
    printf ("#define sc_d1         %d\n", offsetof (struct sigcontext_struct, sc_d1));
    printf ("#define sc_a0         %d\n", offsetof (struct sigcontext_struct, sc_a0));
    printf ("#define sc_a1         %d\n", offsetof (struct sigcontext_struct, sc_a1));
    printf ("#define sc_sr         %d\n", offsetof (struct sigcontext_struct, sc_sr));
    printf ("#define sc_pc         %d\n", offsetof (struct sigcontext_struct, sc_pc));
    printf ("#define sc_formatvec  %d\n", offsetof (struct sigcontext_struct, sc_formatvec));

    printf ("\n/* Task Flags */\n");
    printf ("#define TS_RUN        %d\n", TS_RUN);
    printf ("#define TS_READY      %d\n", TS_READY);
    printf ("#define TF_EXCEPT     0x%04lX\n", TF_EXCEPT);
    printf ("#define TF_SWITCH     0x%04lX\n", TF_SWITCH);

    printf ("#define TB_EXCEPT     %d\n", TB_EXCEPT);
    printf ("#define TB_SWITCH     %d\n", TB_SWITCH);
    printf ("#define TB_LAUNCH     %d\n", TB_LAUNCH);

    printf ("\n/* Exec functions */\n");
    printf ("#define Switch        %d\n", FuncOffset (9));
    printf ("#define Dispatch      %d\n", FuncOffset (10));
    printf ("#define Exception     %d\n", FuncOffset (11));
    printf ("#define Alert         %d\n", FuncOffset (18));
    printf ("#define Disable       %d\n", FuncOffset (20));
    printf ("#define Enable        %d\n", FuncOffset (21));
    printf ("#define Enqueue       %d\n", FuncOffset (45));
    printf ("#define StackSwap     %d\n", FuncOffset (122));

    printf ("\n/* Constants */\n");
    printf ("#define AT_DeadEnd    0x%08X\n", AT_DeadEnd);
    printf ("#define AN_StackProbe 0x%08X\n", AN_StackProbe);

#ifdef UseExecstubs
    printf ("#define UseExecstubs 1\n");
#endif
    return 0;
}
