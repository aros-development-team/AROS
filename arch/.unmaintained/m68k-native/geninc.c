#include <exec/alerts.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <dos/dosextens.h>
#include <stdio.h>
#include <stddef.h>

#define FuncOffset(x)       (int)__AROS_GETJUMPVEC(0,x)

ULONG BitOf(ULONG);

int main (void)
{
    printf ("# Macros\n"
	"#define AROS_CSYMNAME(n)       _ ## n\n"
	"#define AROS_CDEFNAME(n)       _ ## n\n"
	"#define AROS_SLIB_ENTRY(n,s)   _ ## s ## _ ## n\n"
	"\n");

    printf ("# ExecBase\n");
    printf ("\tAttnResched   = %d\n", (int)offsetof (struct ExecBase, AttnResched));
    printf ("\tIDNestCnt     = %d\n", (int)offsetof (struct ExecBase, IDNestCnt));
    printf ("\tTDNestCnt     = %d\n", (int)offsetof (struct ExecBase, TDNestCnt));
    printf ("\tTaskReady     = %d\n", (int)offsetof (struct ExecBase, TaskReady));
    printf ("\tThisTask      = %d\n", (int)offsetof (struct ExecBase, ThisTask));

    printf ("\n# struct Task\n");
    printf ("\ttc_State      = %d\n", (int)offsetof (struct Task, tc_State));
    printf ("\ttc_Flags      = %d\n", (int)offsetof (struct Task, tc_Flags));
    printf ("\ttc_ExceptCode = %d\n", (int)offsetof (struct Task, tc_ExceptCode));
    printf ("\ttc_ExceptData = %d\n", (int)offsetof (struct Task, tc_ExceptData));
    printf ("\ttc_SigExcept  = %d\n", (int)offsetof (struct Task, tc_SigExcept));
    printf ("\ttc_SigRecvd   = %d\n", (int)offsetof (struct Task, tc_SigRecvd));
    printf ("\ttc_Launch     = %d\n", (int)offsetof (struct Task, tc_Launch));
    printf ("\ttc_Switch     = %d\n", (int)offsetof (struct Task, tc_Switch));
    printf ("\ttc_SPReg      = %d\n", (int)offsetof (struct Task, tc_SPReg));
    printf ("\ttc_SPLower    = %d\n", (int)offsetof (struct Task, tc_SPLower));
    printf ("\ttc_SPUpper    = %d\n", (int)offsetof (struct Task, tc_SPUpper));
    printf ("\ttc_IDNestCnt  = %d\n", (int)offsetof (struct Task, tc_IDNestCnt));
    printf ("\ttc_TrapCode   = %d\n", (int)offsetof (struct Task, tc_TrapCode));

    printf ("\n# struct DosBase\n");
    printf ("\tdl_SysBase    = %d\n", (int)offsetof (struct DosLibrary, dl_SysBase));

    printf ("\n# struct StackSwapStruct\n");
    printf ("\tstk_Lower     = %d\n", (int)offsetof (struct StackSwapStruct, stk_Lower));
    printf ("\tstk_Upper     = %d\n", (int)offsetof (struct StackSwapStruct, stk_Upper));
    printf ("\tstk_Pointer   = %d\n", (int)offsetof (struct StackSwapStruct, stk_Pointer));

    printf ("\n# Task Flags\n");
    printf ("\tTS_RUN        = %d\n", TS_RUN);
    printf ("\tTS_READY      = %d\n", TS_READY);
    printf ("\tTF_EXCEPT     = 0x%04lX\n", TF_EXCEPT);
    printf ("\tTF_SWITCH     = 0x%04lX\n", TF_SWITCH);

    printf ("\tTB_EXCEPT     = %d\n", TB_EXCEPT);
    printf ("\tTB_SWITCH     = %d\n", TB_SWITCH);
    printf ("\tTB_LAUNCH     = %d\n", TB_LAUNCH);

    printf ("\n# Exec functions\n");
    printf ("\tSupervisor    = %d\n", FuncOffset (5));
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

    printf ("\n# Cache constants\n");
    printf ("\tCACRF_EnableI       = 0x%08lx\n", CACRF_EnableI      );
    printf ("\tCACRF_FreezeI       = 0x%08lx\n", CACRF_FreezeI      );
    printf ("\tCACRF_ClearI        = 0x%08lx\n", CACRF_ClearI       );
    printf ("\tCACRF_IBE           = 0x%08lx\n", CACRF_IBE          );
    printf ("\tCACRF_EnableD       = 0x%08lx\n", CACRF_EnableD      );
    printf ("\tCACRF_FreezeD       = 0x%08lx\n", CACRF_FreezeD      );
    printf ("\tCACRF_ClearD        = 0x%08lx\n", CACRF_ClearD       );
    printf ("\tCACRF_DBE           = 0x%08lx\n", CACRF_DBE          );
    printf ("\tCACRF_WriteAllocate = 0x%08lx\n", CACRF_WriteAllocate);
    printf ("\tCACRF_EnableE       = 0x%08lx\n", CACRF_EnableE      );
    printf ("\tCACRF_CopyBack      = 0x%08lx\n", CACRF_CopyBack     );
    printf ("\tDMA_Continue        = 0x%08lx\n", DMA_Continue       );
    printf ("\tDMA_NoModify        = 0x%08lx\n", DMA_NoModify       );
    printf ("\tDMA_ReadFromRAM     = 0x%08lx\n", DMA_ReadFromRAM    );

    printf ("\tCACRB_EnableI       = %ld\n", BitOf(CACRF_EnableI      ));
    printf ("\tCACRB_FreezeI       = %ld\n", BitOf(CACRF_FreezeI      ));
    printf ("\tCACRB_ClearI        = %ld\n", BitOf(CACRF_ClearI       ));
    printf ("\tCACRB_IBE           = %ld\n", BitOf(CACRF_IBE          ));
    printf ("\tCACRB_EnableD       = %ld\n", BitOf(CACRF_EnableD      ));
    printf ("\tCACRB_FreezeD       = %ld\n", BitOf(CACRF_FreezeD      ));
    printf ("\tCACRB_ClearD        = %ld\n", BitOf(CACRF_ClearD       ));
    printf ("\tCACRB_DBE           = %ld\n", BitOf(CACRF_DBE          ));
    printf ("\tCACRB_WriteAllocate = %ld\n", BitOf(CACRF_WriteAllocate));
    printf ("\tCACRB_EnableE       = %ld\n", BitOf(CACRF_EnableE      ));
    printf ("\tCACRB_CopyBack      = %ld\n", BitOf(CACRF_CopyBack     ));
    printf ("\tDMAB_Continue       = %ld\n", BitOf(DMA_Continue       ));
    printf ("\tDMAB_NoModify       = %ld\n", BitOf(DMA_NoModify       ));
    printf ("\tDMAB_ReadFromRAM    = %ld\n", BitOf(DMA_ReadFromRAM    ));

    return 0;
}

ULONG BitOf(ULONG mask)
{
    LONG ret = -1;

    do
    {
	mask = mask >> 1;
	ret++;
    } while(mask);

    return (ULONG)ret;
}
