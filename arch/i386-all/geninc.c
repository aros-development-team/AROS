/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/alerts.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <dos/dosextens.h>
#include <stdio.h>
#include <stddef.h>

#define FuncOffset(x)       (int)__AROS_GETJUMPVEC(0,x)

int main (void)
{
    printf ("/* Macros */\n"
#if (defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)) && !defined(__ELF__)
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

    printf ("/* ExecBase */\n");
    printf ("#define AttnResched   %d\n", offsetof (struct ExecBase, AttnResched));
    printf ("#define IDNestCnt     %d\n", offsetof (struct ExecBase, IDNestCnt));
    printf ("#define TDNestCnt     %d\n", offsetof (struct ExecBase, TDNestCnt));
    printf ("#define TaskReady     %d\n", offsetof (struct ExecBase, TaskReady));
    printf ("#define ThisTask      %d\n", offsetof (struct ExecBase, ThisTask));

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

    printf ("\n/* struct StackSwapStruct */\n");
    printf ("#define stk_Lower     %d\n", offsetof (struct StackSwapStruct, stk_Lower));
    printf ("#define stk_Upper     %d\n", offsetof (struct StackSwapStruct, stk_Upper));
    printf ("#define stk_Pointer   %d\n", offsetof (struct StackSwapStruct, stk_Pointer));

    printf ("\n/* Task Flags */\n");
    printf ("#define TS_RUN        %d\n", TS_RUN);
    printf ("#define TS_READY      %d\n", TS_READY);
    printf ("#define TF_EXCEPT     0x%04lX\n", TF_EXCEPT);
    printf ("#define TF_SWITCH     0x%04lX\n", TF_SWITCH);

    printf ("\n/* Exec functions */\n");
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

#ifdef UseExecstubs
    printf ("#define UseExecstubs 1\n");
#endif
    return 0;
}


