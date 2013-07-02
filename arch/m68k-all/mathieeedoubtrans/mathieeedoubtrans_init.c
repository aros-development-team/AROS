/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Init of 68k/FPU mathieeedoubtrans.library
    Lang: english
*/
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <utility/utility.h> /* this must be before mathieeedoubtrans_intern.h */

#include "mathieeedoubtrans_intern.h"

#define SetFunc(a,b) SetFunction((struct Library *)lh, a * -LIB_VECTSIZE, AROS_SLIB_ENTRY(b,MathIeeeDoubTrans,a))

extern void AROS_SLIB_ENTRY(ATan_6888x,MathIeeeDoubTrans,5)(void);
extern void AROS_SLIB_ENTRY(Sin_6888x,MathIeeeDoubTrans,6)(void);
extern void AROS_SLIB_ENTRY(Cos_6888x,MathIeeeDoubTrans,7)(void);
extern void AROS_SLIB_ENTRY(Tan_6888x,MathIeeeDoubTrans,8)(void);
extern void AROS_SLIB_ENTRY(Sincos_6888x,MathIeeeDoubTrans,9)(void);
extern void AROS_SLIB_ENTRY(Sinh_6888x,MathIeeeDoubTrans,10)(void);
extern void AROS_SLIB_ENTRY(Cosh_6888x,MathIeeeDoubTrans,11)(void);
extern void AROS_SLIB_ENTRY(Tanh_6888x,MathIeeeDoubTrans,12)(void);
extern void AROS_SLIB_ENTRY(Exp_6888x,MathIeeeDoubTrans,13)(void);
extern void AROS_SLIB_ENTRY(Log_6888x,MathIeeeDoubTrans,14)(void);
extern void AROS_SLIB_ENTRY(Pow_6888x,MathIeeeDoubTrans,15)(void);
extern void AROS_SLIB_ENTRY(Sqrt_6888x,MathIeeeDoubTrans,16)(void);
extern void AROS_SLIB_ENTRY(Tieee_6888x,MathIeeeDoubTrans,17)(void);
extern void AROS_SLIB_ENTRY(Fieee_6888x,MathIeeeDoubTrans,18)(void);
extern void AROS_SLIB_ENTRY(Asin_6888x,MathIeeeDoubTrans,19)(void);
extern void AROS_SLIB_ENTRY(Acos_6888x,MathIeeeDoubTrans,20)(void);
extern void AROS_SLIB_ENTRY(Log10_6888x,MathIeeeDoubTrans,21)(void);

struct Library *MathIeeeDoubBasBase;

static int IEEEDPT_Init(struct MathIeeeDoubTransBase *lh)
{
    MathIeeeDoubBasBase = OpenLibrary("mathieeedoubbas.library", 39);
    if (!MathIeeeDoubBasBase)
	return FALSE;

    if (SysBase->AttnFlags & (AFF_68881 | AFF_68882 | AFF_FPU40)) {
	SetFunc(5, ATan_6888x);
	SetFunc(6, Sin_6888x);
	SetFunc(7, Cos_6888x);
	SetFunc(8, Tan_6888x);
	SetFunc(9, Sincos_6888x);
	SetFunc(10, Sinh_6888x);
	SetFunc(11, Cosh_6888x);
	SetFunc(12, Tanh_6888x);
	SetFunc(13, Exp_6888x);
	SetFunc(14, Log_6888x);
	/* SetFunc(15, Pow_6888x); */
	SetFunc(16, Sqrt_6888x);
	SetFunc(17, Tieee_6888x);
	SetFunc(18, Fieee_6888x);
	SetFunc(19, Asin_6888x);
	SetFunc(20, Acos_6888x);
	SetFunc(21, Log10_6888x);
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
