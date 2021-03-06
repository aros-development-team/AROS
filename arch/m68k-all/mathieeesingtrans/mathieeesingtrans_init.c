/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

    Desc: Init of 68k/FPU mathieeesingtrans.library
*/
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <utility/utility.h> /* this must be before mathieeesingtrans_intern.h */

#include <defines/mathieeesingtrans_LVO.h>

#include "mathieeesingtrans_intern.h"

#define SetFunc(a,b) SetFunction((struct Library *)lh, -(a) * LIB_VECTSIZE, AROS_SLIB_ENTRY(b,MathIeeeSingTrans,a))

extern void AROS_SLIB_ENTRY(ATan_6888x,MathIeeeSingTrans,LVOIEEESPAtan)(void);
extern void AROS_SLIB_ENTRY(Sin_6888x,MathIeeeSingTrans,LVOIEEESPSin)(void);
extern void AROS_SLIB_ENTRY(Cos_6888x,MathIeeeSingTrans,LVOIEEESPCos)(void);
extern void AROS_SLIB_ENTRY(Tan_6888x,MathIeeeSingTrans,LVOIEEESPTan)(void);
extern void AROS_SLIB_ENTRY(Sincos_6888x,MathIeeeSingTrans,LVOIEEESPSincos)(void);
extern void AROS_SLIB_ENTRY(Sinh_6888x,MathIeeeSingTrans,LVOIEEESPSinh)(void);
extern void AROS_SLIB_ENTRY(Cosh_6888x,MathIeeeSingTrans,LVOIEEESPCosh)(void);
extern void AROS_SLIB_ENTRY(Tanh_6888x,MathIeeeSingTrans,LVOIEEESPTanh)(void);
extern void AROS_SLIB_ENTRY(Exp_6888x,MathIeeeSingTrans,LVOIEEESPExp)(void);
extern void AROS_SLIB_ENTRY(Log_6888x,MathIeeeSingTrans,LVOIEEESPLog)(void);
extern void AROS_SLIB_ENTRY(Pow_6888x,MathIeeeSingTrans,LVOIEEESPPow)(void);
extern void AROS_SLIB_ENTRY(Sqrt_6888x,MathIeeeSingTrans,LVOIEEESPSqrt)(void);
extern void AROS_SLIB_ENTRY(Tieee_6888x,MathIeeeSingTrans,LVOIEEESPTieee)(void);
extern void AROS_SLIB_ENTRY(Fieee_6888x,MathIeeeSingTrans,LVOIEEESPFieee)(void);
extern void AROS_SLIB_ENTRY(Asin_6888x,MathIeeeSingTrans,LVOIEEESPAsin)(void);
extern void AROS_SLIB_ENTRY(Acos_6888x,MathIeeeSingTrans,LVOIEEESPAcos)(void);
extern void AROS_SLIB_ENTRY(Log10_6888x,MathIeeeSingTrans,LVOIEEESPLog10)(void);

struct Library * MathIeeeSingBasBase;

static int IEEESPT_Init(struct Library *lh)
{
    MathIeeeSingBasBase = OpenLibrary ("mathieeesingbas.library", 39);
    if (!MathIeeeSingBasBase)
        return FALSE;

    if (SysBase->AttnFlags & (AFF_68881 | AFF_68882 | AFF_FPU40)) {
        SetFunc(LVOIEEESPAtan,          ATan_6888x);
        SetFunc(LVOIEEESPSin,           Sin_6888x);
        SetFunc(LVOIEEESPCos,           Cos_6888x);
        SetFunc(LVOIEEESPTan,           Tan_6888x);
        SetFunc(LVOIEEESPSincos,        Sincos_6888x);
        SetFunc(LVOIEEESPSinh,          Sinh_6888x);
        SetFunc(LVOIEEESPCosh,          Cosh_6888x);
        SetFunc(LVOIEEESPTanh,          Tanh_6888x);
        SetFunc(LVOIEEESPExp,           Exp_6888x);
        SetFunc(LVOIEEESPLog,           Log_6888x);
        /* SetFunc(LVOIEEESPPow,        Pow_6888x); */
        SetFunc(LVOIEEESPSqrt,          Sqrt_6888x);
        /* SetFunc(LVOIEEESPTieee,      Tieee_6888x); */
        /* SetFunc(LVOIEEESPFieee,      Fieee_6888x); */
        SetFunc(LVOIEEESPAsin,          Asin_6888x);
        SetFunc(LVOIEEESPAcos,          Acos_6888x);
        SetFunc(LVOIEEESPLog10,         Log10_6888x);
    }
    return TRUE;
}

static int IEEESPT_Expunge(struct Library *lh)
{
    if (MathIeeeSingBasBase)
        CloseLibrary ((struct Library *)MathIeeeSingBasBase);
    
    return TRUE;
}

ADD2INITLIB(IEEESPT_Init, 0)
ADD2EXPUNGELIB(IEEESPT_Expunge, 0);
