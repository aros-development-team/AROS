/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

    Desc: Init of 68k/FPU mathieeedoubbas.library
*/
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <utility/utility.h> /* this must be before mathieeedoubbas_intern.h */

#include "mathieeedoubbas_intern.h"

#include <defines/mathieeedoubbas_LVO.h>

#define SetFunc(a,b) SetFunction((struct Library *)lh, -(a) * LIB_VECTSIZE, AROS_SLIB_ENTRY(b,MathIeeeDoubBas,a))

extern void AROS_SLIB_ENTRY(Fix_6888x,MathIeeeDoubBas,LVOIEEEDPFix)(void);
extern void AROS_SLIB_ENTRY(Flt_6888x,MathIeeeDoubBas,LVOIEEEDPFlt)(void);

extern void AROS_SLIB_ENTRY(Abs_6888x,MathIeeeDoubBas,LVOIEEEDPAbs)(void);
extern void AROS_SLIB_ENTRY(Neg_6888x,MathIeeeDoubBas,LVOIEEEDPNeg)(void);
extern void AROS_SLIB_ENTRY(Add_6888x,MathIeeeDoubBas,LVOIEEEDPAdd)(void);
extern void AROS_SLIB_ENTRY(Sub_6888x,MathIeeeDoubBas,LVOIEEEDPSub)(void);
extern void AROS_SLIB_ENTRY(Mul_6888x,MathIeeeDoubBas,LVOIEEEDPMul)(void);
extern void AROS_SLIB_ENTRY(Div_6888x,MathIeeeDoubBas,LVOIEEEDPDiv)(void);
extern void init_6888x_double(void);

static int IEEEDP_Open(struct MathIeeeDoubBasBase *lh)
{
    if (SysBase->AttnFlags & (AFF_68881 | AFF_68882 | AFF_FPU40))
        init_6888x_double();
    return TRUE;
}

static int IEEEDP_Init(struct MathIeeeDoubBasBase *lh)
{
    if (SysBase->AttnFlags & (AFF_68881 | AFF_68882 | AFF_FPU40)) {
        SetFunc(LVOIEEEDPFix,           Fix_6888x);
        SetFunc(LVOIEEEDPFlt,           Flt_6888x);
        //SetFunc(LVOIEEEDPCmp,         Cmp_6888x); use software
        //SetFunc(LVOIEEEDPTst,         Tst_6888x); use software
        SetFunc(LVOIEEEDPAbs,           Abs_6888x);
        SetFunc(LVOIEEEDPNeg,           Neg_6888x);
        SetFunc(LVOIEEEDPAdd,           Add_6888x);
        SetFunc(LVOIEEEDPSub,           Sub_6888x);
        SetFunc(LVOIEEEDPMul,           Mul_6888x);
        SetFunc(LVOIEEEDPDiv,           Div_6888x);
        //SetFunc(LVOIEEEDPFloor,       Floor_6888x); use software
        //SetFunc(LVOIEEEDPCeil,        Ceil_6888x); use software
    }
    return TRUE;
}

ADD2INITLIB(IEEEDP_Init, 0)
ADD2OPENLIB(IEEEDP_Open, 0)
