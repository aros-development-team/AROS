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
    printf ("# Macros\n"
#ifdef __FreeBSD__
	"#define AROS_CSYMNAME(n)       _ ## n\n"
	"#define AROS_CDEFNAME(n)       _ ## n\n"
	"#define AROS_SLIB_ENTRY(n,s)   _ ## s ## _ ## n\n"
#else
	"#define AROS_CSYMNAME(n)       n\n"
	"#define AROS_CDEFNAME(n)       n\n"
	"#define AROS_SLIB_ENTRY(n,s)   s ## _ ## n\n"
#endif
	    "\n");

    printf ("# For sigprocmask\n");
    printf ("\tSIG_BLOCK     = %d\n", SIG_BLOCK);
    printf ("\tSIG_UNBLOCK   = %d\n\n", SIG_UNBLOCK);

    printf ("# ExecBase\n");
    printf ("\tAttnResched   = %d\n", offsetof (struct ExecBase, AttnResched));
    printf ("\tIDNestCnt     = %d\n", offsetof (struct ExecBase, IDNestCnt));
    printf ("\tTDNestCnt     = %d\n", offsetof (struct ExecBase, TDNestCnt));
    printf ("\tTaskReady     = %d\n", offsetof (struct ExecBase, TaskReady));
    printf ("\tThisTask      = %d\n", offsetof (struct ExecBase, ThisTask));

    printf ("\n# struct Task\n");
    printf ("\ttc_State      = %d\n", offsetof (struct Task, tc_State));
    printf ("\ttc_Flags      = %d\n", offsetof (struct Task, tc_Flags));
    printf ("\ttc_ExceptCode = %d\n", offsetof (struct Task, tc_ExceptCode));
    printf ("\ttc_ExceptData = %d\n", offsetof (struct Task, tc_ExceptData));
    printf ("\ttc_SigExcept  = %d\n", offsetof (struct Task, tc_SigExcept));
    printf ("\ttc_SigRecvd   = %d\n", offsetof (struct Task, tc_SigRecvd));
    printf ("\ttc_Launch     = %d\n", offsetof (struct Task, tc_Launch));
    printf ("\ttc_Switch     = %d\n", offsetof (struct Task, tc_Switch));
    printf ("\ttc_SPReg      = %d\n", offsetof (struct Task, tc_SPReg));
    printf ("\ttc_SPLower    = %d\n", offsetof (struct Task, tc_SPLower));
    printf ("\ttc_SPUpper    = %d\n", offsetof (struct Task, tc_SPUpper));
    printf ("\ttc_IDNestCnt  = %d\n", offsetof (struct Task, tc_IDNestCnt));

    printf ("\n# struct DosBase\n");
    printf ("\tdl_SysBase    = %d\n", offsetof (struct DosLibrary, dl_SysBase));

    printf ("\n# struct StackSwapStruct\n");
    printf ("\tstk_Lower     = %d\n", offsetof (struct StackSwapStruct, stk_Lower));
    printf ("\tstk_Upper     = %d\n", offsetof (struct StackSwapStruct, stk_Upper));
    printf ("\tstk_Pointer   = %d\n", offsetof (struct StackSwapStruct, stk_Pointer));

    printf ("\n# Task Flags\n");
    printf ("\tTS_RUN        = %d\n", TS_RUN);
    printf ("\tTS_READY      = %d\n", TS_READY);
    printf ("\tTF_EXCEPT     = 0x%04lX\n", TF_EXCEPT);
    printf ("\tTF_SWITCH     = 0x%04lX\n", TF_SWITCH);

    printf ("\n# Exec functions\n");
    printf ("\tSwitch        = %d\n", FuncOffset (9));
    printf ("\tDispatch      = %d\n", FuncOffset (10));
    printf ("\tException     = %d\n", FuncOffset (11));
    printf ("\tAlert         = %d\n", FuncOffset (18));
    printf ("\tDisable       = %d\n", FuncOffset (20));
    printf ("\tEnable        = %d\n", FuncOffset (21));
    printf ("\tEnqueue       = %d\n", FuncOffset (45));
    printf ("\tStackSwap     = %d\n", FuncOffset (122));

    printf ("\n# Constants\n");
    printf ("\tAT_DeadEnd    = 0x%08X\n", AT_DeadEnd);
    printf ("\tAN_StackProbe = 0x%08X\n", AN_StackProbe);

    return 0;
}


