/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/alerts.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <dos/dosextens.h>
#include <hardware/custom.h>
#include <graphics/rastport.h>
#include <graphics/clip.h>
#include "privheaders.h"
#include <stdio.h>
#include <stddef.h>

#define FuncOffset(x)       (int)__AROS_GETJUMPVEC(0,x)

ULONG BitOf(ULONG);

int main (void)
{
    printf ("/*\n    This file is generated automatically!\n\n"
	    "    If you want to make changes, edit \"geninc.c\" and delete this file.\n*/\n\n");

    printf ("/* Macros */\n"
	"#define AROS_CSYMNAME(n)       _ ## n\n"
	"#define AROS_CDEFNAME(n)       _ ## n\n"
	"#define AROS_SLIB_ENTRY(n,s)   _ ## s ## _ ## n\n"
	"\n");

    printf ("/* ExecBase */\n");
    printf ("#define AttnResched   %d\n", (int)offsetof (struct ExecBase, AttnResched));
    printf ("#define IDNestCnt     %d\n", (int)offsetof (struct ExecBase, IDNestCnt));
    printf ("#define TDNestCnt     %d\n", (int)offsetof (struct ExecBase, TDNestCnt));
    printf ("#define TaskReady     %d\n", (int)offsetof (struct ExecBase, TaskReady));
    printf ("#define ThisTask      %d\n", (int)offsetof (struct ExecBase, ThisTask));

    printf ("\n/* struct Task */\n");
    printf ("#define tc_State      %d\n", (int)offsetof (struct Task, tc_State));
    printf ("#define tc_Flags      %d\n", (int)offsetof (struct Task, tc_Flags));
    printf ("#define tc_ExceptCode %d\n", (int)offsetof (struct Task, tc_ExceptCode));
    printf ("#define tc_ExceptData %d\n", (int)offsetof (struct Task, tc_ExceptData));
    printf ("#define tc_SigExcept  %d\n", (int)offsetof (struct Task, tc_SigExcept));
    printf ("#define tc_SigRecvd   %d\n", (int)offsetof (struct Task, tc_SigRecvd));
    printf ("#define tc_Launch     %d\n", (int)offsetof (struct Task, tc_Launch));
    printf ("#define tc_Switch     %d\n", (int)offsetof (struct Task, tc_Switch));
    printf ("#define tc_SPReg      %d\n", (int)offsetof (struct Task, tc_SPReg));
    printf ("#define tc_SPLower    %d\n", (int)offsetof (struct Task, tc_SPLower));
    printf ("#define tc_SPUpper    %d\n", (int)offsetof (struct Task, tc_SPUpper));
    printf ("#define tc_IDNestCnt  %d\n", (int)offsetof (struct Task, tc_IDNestCnt));
    printf ("#define tc_TrapCode   %d\n", (int)offsetof (struct Task, tc_TrapCode));

    printf ("\n/* struct DosBase */\n");
    printf ("#define dl_SysBase    %d\n", (int)offsetof (struct DosLibrary, dl_SysBase));

    printf ("\n/* struct StackSwapStruct */\n");
    printf ("#define stk_Lower     %d\n", (int)offsetof (struct StackSwapStruct, stk_Lower));
    printf ("#define stk_Upper     %d\n", (int)offsetof (struct StackSwapStruct, stk_Upper));
    printf ("#define stk_Pointer   %d\n", (int)offsetof (struct StackSwapStruct, stk_Pointer));

    printf ("\n/* Task Flags */\n");
    printf ("#define TS_RUN        %d\n", TS_RUN);
    printf ("#define TS_READY      %d\n", TS_READY);
    printf ("#define TF_EXCEPT     0x%04lX\n", TF_EXCEPT);
    printf ("#define TF_SWITCH     0x%04lX\n", TF_SWITCH);

    printf ("#define TB_EXCEPT     %d\n", TB_EXCEPT);
    printf ("#define TB_SWITCH     %d\n", TB_SWITCH);
    printf ("#define TB_LAUNCH     %d\n", TB_LAUNCH);

    printf ("\n/* Exec functions */\n");
    printf ("#define Supervisor    %d\n", FuncOffset (5));
    printf ("#define Switch        %d\n", FuncOffset (9));
    printf ("#define Dispatch      %d\n", FuncOffset (10));
    printf ("#define Exception     %d\n", FuncOffset (11));
    printf ("#define Alert         %d\n", FuncOffset (18));
    printf ("#define Disable       %d\n", FuncOffset (20));
    printf ("#define Enable        %d\n", FuncOffset (21));
    printf ("#define Enqueue       %d\n", FuncOffset (45));
    printf ("#define CacheControl  %d\n", FuncOffset (107));
    printf ("#define StackSwap     %d\n", FuncOffset (122));

    printf ("\n/* Constants */\n");
    printf ("#define AT_DeadEnd    0x%08X\n", AT_DeadEnd);
    printf ("#define AN_StackProbe 0x%08X\n", AN_StackProbe);

    printf ("\n/* Cache constants */\n");
    printf ("#define CACRF_EnableI        0x%08lx\n", CACRF_EnableI      );
    printf ("#define CACRF_FreezeI        0x%08lx\n", CACRF_FreezeI      );
    printf ("#define CACRF_ClearI         0x%08lx\n", CACRF_ClearI       );
    printf ("#define CACRF_IBE            0x%08lx\n", CACRF_IBE          );
    printf ("#define CACRF_EnableD        0x%08lx\n", CACRF_EnableD      );
    printf ("#define CACRF_FreezeD        0x%08lx\n", CACRF_FreezeD      );
    printf ("#define CACRF_ClearD         0x%08lx\n", CACRF_ClearD       );
    printf ("#define CACRF_DBE            0x%08lx\n", CACRF_DBE          );
    printf ("#define CACRF_WriteAllocate  0x%08lx\n", CACRF_WriteAllocate);
    printf ("#define CACRF_EnableE        0x%08lx\n", CACRF_EnableE      );
    printf ("#define CACRF_CopyBack       0x%08lx\n", CACRF_CopyBack     );
    printf ("#define DMA_Continue         0x%08lx\n", DMA_Continue       );
    printf ("#define DMA_NoModify         0x%08lx\n", DMA_NoModify       );
    printf ("#define DMA_ReadFromRA       0x%08lx\n", DMA_ReadFromRAM    );

    printf ("\n#define CACRB_EnableI        %ld\n", BitOf(CACRF_EnableI      ));
    printf ("#define CACRB_FreezeI        %ld\n", BitOf(CACRF_FreezeI      ));
    printf ("#define CACRB_ClearI         %ld\n", BitOf(CACRF_ClearI       ));
    printf ("#define CACRB_IBE            %ld\n", BitOf(CACRF_IBE          ));
    printf ("#define CACRB_EnableD        %ld\n", BitOf(CACRF_EnableD      ));
    printf ("#define CACRB_FreezeD        %ld\n", BitOf(CACRF_FreezeD      ));
    printf ("#define CACRB_ClearD         %ld\n", BitOf(CACRF_ClearD       ));
    printf ("#define CACRB_DBE            %ld\n", BitOf(CACRF_DBE          ));
    printf ("#define CACRB_WriteAllocate  %ld\n", BitOf(CACRF_WriteAllocate));
    printf ("#define CACRB_EnableE        %ld\n", BitOf(CACRF_EnableE      ));
    printf ("#define CACRB_CopyBack       %ld\n", BitOf(CACRF_CopyBack     ));
    printf ("#define DMAB_Continue        %ld\n", BitOf(DMA_Continue       ));
    printf ("#define DMAB_NoModify        %ld\n", BitOf(DMA_NoModify       ));
    printf ("#define DMAB_ReadFromR       %ld\n", BitOf(DMA_ReadFromRAM    ));

    printf ("\n/* Custom register constants */\n");
    printf ("#define custom               0xdff000\n");
    printf ("#define dmaconr              0x%04lx\n", offsetof (struct Custom, dmaconr));
    printf ("#define intenar              0x%04lx\n", offsetof (struct Custom, intenar));
    printf ("#define intreqr              0x%04lx\n", offsetof (struct Custom, intreqr));
    printf ("#define intena               0x%04lx\n", offsetof (struct Custom, intena));
    printf ("#define intreq               0x%04lx\n", offsetof (struct Custom, intreq));
    printf ("#define dmacon               0x%04lx\n", offsetof (struct Custom, dmacon));

    printf ("\n/* Graphics constants */\n");
    printf ("#define BltBitMap            %d\n", FuncOffset (5));

    printf ("\n#define lb_GfxBase           %d\n", (int)offsetof (struct LayersBase, lb_GfxBase));
    printf ("#define lb_SysBase           %d\n", (int)offsetof (struct LayersBase, lb_SysBase));

    printf ("\n#define rp_Layer             %d\n", (int)offsetof (struct RastPort, Layer));
    printf ("#define rp_BitMap            %d\n", (int)offsetof (struct RastPort, BitMap));
    printf ("#define cr_BitMap            %d\n", (int)offsetof (struct ClipRect, BitMap));
    printf ("#define cr_MinX              %d\n", (int)offsetof (struct ClipRect, bounds.MinX));
    printf ("#define cr_MinY              %d\n", (int)offsetof (struct ClipRect, bounds.MinY));
    printf ("#define cr_MaxX              %d\n", (int)offsetof (struct ClipRect, bounds.MaxX));
    printf ("#define cr_MaxY              %d\n", (int)offsetof (struct ClipRect, bounds.MaxY));

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
