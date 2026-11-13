/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

    Desc: Init of 68k/FPU mathieeesingbas.library
*/
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <utility/utility.h> /* this must be before mathieeesingbas_intern.h */

#include "mathieeesingbas_intern.h"

#include <defines/mathieeesingbas_LVO.h>

#define SetFunc(a,b) SetFunction(lh, -(a) * LIB_VECTSIZE, AROS_SLIB_ENTRY(b,MathIeeeSingBas,a))

extern void AROS_SLIB_ENTRY(Fix_6888x,MathIeeeSingBas,LVOIEEESPFix)(void);
extern void AROS_SLIB_ENTRY(Flt_6888x,MathIeeeSingBas,LVOIEEESPFlt)(void);
extern void AROS_SLIB_ENTRY(Abs_6888x,MathIeeeSingBas,LVOIEEESPAbs)(void);
extern void AROS_SLIB_ENTRY(Neg_6888x,MathIeeeSingBas,LVOIEEESPNeg)(void);
extern void AROS_SLIB_ENTRY(Add_6888x,MathIeeeSingBas,LVOIEEESPAdd)(void);
extern void AROS_SLIB_ENTRY(Sub_6888x,MathIeeeSingBas,LVOIEEESPSub)(void);
extern void AROS_SLIB_ENTRY(Mul_6888x,MathIeeeSingBas,LVOIEEESPMul)(void);
extern void AROS_SLIB_ENTRY(Div_6888x,MathIeeeSingBas,LVOIEEESPDiv)(void);

extern void init_6888x_single(void);

static int IEEESP_Open(struct Library *lh)
{
    if (SysBase->AttnFlags & (AFF_68881 | AFF_68882 | AFF_FPU40))
        init_6888x_single();
    return TRUE;
}

static int IEEESP_Init(struct Library *lh)
{
    if (SysBase->AttnFlags & (AFF_68881 | AFF_68882 | AFF_FPU40)) {
        SetFunc(LVOIEEESPFix,           Fix_6888x);
        SetFunc(LVOIEEESPFlt,           Flt_6888x);
        //SetFunc(LVOIEEESPCmp,         Cmp_6888x); use software
        //SetFunc(LVOIEEESPTst,         Tst_6888x); use software
        SetFunc(LVOIEEESPAbs,           Abs_6888x);
        SetFunc(LVOIEEESPNeg,           Neg_6888x);
        SetFunc(LVOIEEESPAdd,           Add_6888x);
        SetFunc(LVOIEEESPSub,           Sub_6888x);
        SetFunc(LVOIEEESPMul,           Mul_6888x);
        SetFunc(LVOIEEESPDiv,           Div_6888x);
        //SetFunc(LVOIEEESPFloor,       Floor_6888x); use software
        //SetFunc(LVOIEEESPCeil,        Ceil_6888x); use software
    }
    return TRUE;
}

ADD2INITLIB(IEEESP_Init, 0)
ADD2OPENLIB(IEEESP_Open, 0)
