#include <exec/alerts.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <dos/dosextens.h>
#include <stdio.h>
#include <stddef.h>
#include <signal.h>

#define FuncOffset(x)       (int)__AROS_GETJUMPVEC(0,x)

int main (void)
{
    printf ("/* Macros */\n"
#if (defined(__FreeBSD__) || defined(__NetBSD__)) && !defined(__ELF__)
	"#define AROS_CSYMNAME(n)       _ ## n\n"
	"#define AROS_CDEFNAME(n)       _ ## n\n"
	"#define AROS_SLIB_ENTRY(n,s)   _ ## s ## _ ## n\n"
#else
	"#define AROS_CSYMNAME(n)       n\n"
	"#define AROS_CDEFNAME(n)       n\n"
	"#define AROS_SLIB_ENTRY(n,s)   s ## _ ## n\n"
#endif
#ifndef __CYGWIN32__
	"#define _FUNCTION(n)           .type   n,@function\n"
	"#define _ALIGNMENT             .balign 16\n"

#else
	"#define _FUNCTION(n)           .def    n; .scl 2; .type 32; .endef\n"
	"#define _ALIGNMENT             .align 4\n"
#endif
	    "\n");

    printf ("/* For sigprocmask */\n");
    printf ("#define SIG_BLOCK     %d\n", SIG_BLOCK);
    printf ("#define SIG_UNBLOCK   %d\n\n", SIG_UNBLOCK);

    printf ("/* ExecBase */\n");
    printf ("#define AttnResched   %d\n", offsetof (struct ExecBase, AttnResched));
    printf ("#define IDNestCnt     %d\n", offsetof (struct ExecBase, IDNestCnt));
    printf ("#define TDNestCnt     %d\n", offsetof (struct ExecBase, TDNestCnt));
    printf ("#define TaskReady     %d\n", offsetof (struct ExecBase, TaskReady));
    printf ("#define ThisTask      %d\n", offsetof (struct ExecBase, ThisTask));
    printf ("#define SysFlags      %d\n", offsetof (struct ExecBase, SysFlags));
    printf ("#define IdleCount     %d\n", offsetof (struct ExecBase, IdleCount));
    printf ("#define DispCount     %d\n", offsetof (struct ExecBase, DispCount));
    printf ("#define Quantum       %d\n", offsetof (struct ExecBase, Quantum));
    printf ("#define Elapsed       %d\n", offsetof (struct ExecBase, Elapsed));

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
    printf ("#define tc_ETask      %d\n", offsetof (struct Task, tc_UnionETask.tc_ETask));
    printf ("#define iet_Context   %d\n", sizeof (struct ETask) + 4);

    printf ("\n/* struct DosBase */\n");
    printf ("#define dl_SysBase    %d\n", offsetof (struct DosLibrary, dl_SysBase));

    printf ("\n/* struct StackSwapStruct */\n");
    printf ("#define stk_Lower     %d\n", offsetof (struct StackSwapStruct, stk_Lower));
    printf ("#define stk_Upper     %d\n", offsetof (struct StackSwapStruct, stk_Upper));
    printf ("#define stk_Pointer   %d\n", offsetof (struct StackSwapStruct, stk_Pointer));

    printf ("\n/* Task Flags */\n");
    printf ("#define TS_RUN        %d\n", TS_RUN);
    printf ("#define TS_READY      %d\n", TS_READY);
    printf ("#define TF_EXCEPT     0x%04lX\n", TF_EXCEPT);
    printf ("#define TF_SWITCH     0x%04lX\n", TF_SWITCH);
    printf ("#define TF_LAUNCH     0x%04lX\n", TF_LAUNCH);

    printf ("\n/* Exec functions */\n");
    printf ("#define Reschedule    %d\n", FuncOffset (8));
    printf ("#define Switch        %d\n", FuncOffset (9));
    printf ("#define Dispatch      %d\n", FuncOffset (10));
    printf ("#define Exception     %d\n", FuncOffset (11));
    printf ("#define Alert         %d\n", FuncOffset (18));
    printf ("#define Disable       %d\n", FuncOffset (20));
    printf ("#define Enable        %d\n", FuncOffset (21));
    printf ("#define Enqueue       %d\n", FuncOffset (45));
    printf ("#define FindTask	   %d\n", FuncOffset (49));
    printf ("#define StackSwap     %d\n", FuncOffset (122));

    printf ("\n/* Constants */\n");
    printf ("#define AT_DeadEnd    0x%08X\n", AT_DeadEnd);
    printf ("#define AN_StackProbe 0x%08X\n", AN_StackProbe);

    printf ("#define ln_Succ       %d\n", offsetof (struct Node, ln_Succ));
    printf ("#define ln_Pred       %d\n", offsetof (struct Node, ln_Pred));
    printf ("#define ln_Pri        %d\n", offsetof (struct Node, ln_Pri));

    printf ("#define lh_Head       %d\n", offsetof (struct List, lh_Head));
    printf ("#define lh_TailPred   %d\n", offsetof (struct List, lh_TailPred));


#ifdef UseExecstubs
    printf ("#define UseExecstubs 1\n");
#endif
    return 0;
}


