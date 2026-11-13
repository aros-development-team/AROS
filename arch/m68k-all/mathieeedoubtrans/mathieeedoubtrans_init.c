/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

    Desc: Init of 68k/FPU mathieeedoubtrans.library
*/
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <utility/utility.h> /* this must be before mathieeedoubtrans_intern.h */

#include "mathieeedoubtrans_intern.h"

#include <defines/mathieeedoubtrans_LVO.h>

#define SetFunc(a,b) SetFunction((struct Library *)lh, -(a) * LIB_VECTSIZE, AROS_SLIB_ENTRY(b,MathIeeeDoubTrans,a))

extern void AROS_SLIB_ENTRY(ATan_6888x,MathIeeeDoubTrans,LVOIEEEDPAtan)(void);
extern void AROS_SLIB_ENTRY(Sin_6888x,MathIeeeDoubTrans,LVOIEEEDPSin)(void);
extern void AROS_SLIB_ENTRY(Cos_6888x,MathIeeeDoubTrans,LVOIEEEDPCos)(void);
extern void AROS_SLIB_ENTRY(Tan_6888x,MathIeeeDoubTrans,LVOIEEEDPTan)(void);
extern void AROS_SLIB_ENTRY(Sincos_6888x,MathIeeeDoubTrans,LVOIEEEDPSincos)(void);
extern void AROS_SLIB_ENTRY(Sinh_6888x,MathIeeeDoubTrans,LVOIEEEDPSinh)(void);
extern void AROS_SLIB_ENTRY(Cosh_6888x,MathIeeeDoubTrans,LVOIEEEDPCosh)(void);
extern void AROS_SLIB_ENTRY(Tanh_6888x,MathIeeeDoubTrans,LVOIEEEDPTanh)(void);
extern void AROS_SLIB_ENTRY(Exp_6888x,MathIeeeDoubTrans,LVOIEEEDPExp)(void);
extern void AROS_SLIB_ENTRY(Log_6888x,MathIeeeDoubTrans,LVOIEEEDPLog)(void);
extern void AROS_SLIB_ENTRY(Pow_6888x,MathIeeeDoubTrans,LVOIEEEDPPow)(void);
extern void AROS_SLIB_ENTRY(Sqrt_6888x,MathIeeeDoubTrans,LVOIEEEDPSqrt)(void);
extern void AROS_SLIB_ENTRY(Tieee_6888x,MathIeeeDoubTrans,LVOIEEEDPTieee)(void);
extern void AROS_SLIB_ENTRY(Fieee_6888x,MathIeeeDoubTrans,LVOIEEEDPFieee)(void);
extern void AROS_SLIB_ENTRY(Asin_6888x,MathIeeeDoubTrans,LVOIEEEDPAsin)(void);
extern void AROS_SLIB_ENTRY(Acos_6888x,MathIeeeDoubTrans,LVOIEEEDPAcos)(void);
extern void AROS_SLIB_ENTRY(Log10_6888x,MathIeeeDoubTrans,LVOIEEEDPLog10)(void);

struct Library *MathIeeeDoubBasBase;

static int IEEEDPT_Init(struct MathIeeeDoubTransBase *lh)
{
    MathIeeeDoubBasBase = OpenLibrary("mathieeedoubbas.library", 39);
    if (!MathIeeeDoubBasBase)
        return FALSE;

    if (SysBase->AttnFlags & (AFF_68881 | AFF_68882 | AFF_FPU40)) {
        SetFunc(LVOIEEEDPAtan,          ATan_6888x);
        SetFunc(LVOIEEEDPSin,           Sin_6888x);
        SetFunc(LVOIEEEDPCos,           Cos_6888x);
        SetFunc(LVOIEEEDPTan,           Tan_6888x);
        SetFunc(LVOIEEEDPSincos,        Sincos_6888x);
        SetFunc(LVOIEEEDPSinh,          Sinh_6888x);
        SetFunc(LVOIEEEDPCosh,          Cosh_6888x);
        SetFunc(LVOIEEEDPTanh,          Tanh_6888x);
        SetFunc(LVOIEEEDPExp,           Exp_6888x);
        SetFunc(LVOIEEEDPLog,           Log_6888x);
        /* SetFunc(LVOIEEEDPPow,        Pow_6888x); */
        SetFunc(LVOIEEEDPSqrt,          Sqrt_6888x);
        SetFunc(LVOIEEEDPTieee,         Tieee_6888x);
        SetFunc(LVOIEEEDPFieee,         Fieee_6888x);
        SetFunc(LVOIEEEDPAsin,          Asin_6888x);
        SetFunc(LVOIEEEDPAcos,          Acos_6888x);
        SetFunc(LVOIEEEDPLog10,         Log10_6888x);
    }
    return TRUE;
}

static int IEEEDPT_Expunge(struct Library *lh)
{
    if (MathIeeeDoubBasBase)
        CloseLibrary (MathIeeeDoubBasBase);
    return TRUE;
}

ADD2INITLIB(IEEEDPT_Init, 0)
ADD2EXPUNGELIB(IEEEDPT_Expunge, 0);
