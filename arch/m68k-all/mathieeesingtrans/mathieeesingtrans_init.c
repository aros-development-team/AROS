/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Init of 68k/FPU mathieeesingtrans.library
    Lang: english
*/
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <utility/utility.h> /* this must be before mathieeesingtrans_intern.h */

#include "mathieeesingtrans_intern.h"

#define SetFunc(a,b) SetFunction((struct Library *)lh, a * -LIB_VECTSIZE, AROS_SLIB_ENTRY(b,MathIeeeSingTrans,a))

extern void AROS_SLIB_ENTRY(ATan_6888x,MathIeeeSingTrans,5)(void);
extern void AROS_SLIB_ENTRY(Sin_6888x,MathIeeeSingTrans,6)(void);
extern void AROS_SLIB_ENTRY(Cos_6888x,MathIeeeSingTrans,7)(void);
extern void AROS_SLIB_ENTRY(Tan_6888x,MathIeeeSingTrans,8)(void);
extern void AROS_SLIB_ENTRY(Sincos_6888x,MathIeeeSingTrans,9)(void);
extern void AROS_SLIB_ENTRY(Sinh_6888x,MathIeeeSingTrans,10)(void);
extern void AROS_SLIB_ENTRY(Cosh_6888x,MathIeeeSingTrans,11)(void);
extern void AROS_SLIB_ENTRY(Tanh_6888x,MathIeeeSingTrans,12)(void);
extern void AROS_SLIB_ENTRY(Exp_6888x,MathIeeeSingTrans,13)(void);
extern void AROS_SLIB_ENTRY(Log_6888x,MathIeeeSingTrans,14)(void);
extern void AROS_SLIB_ENTRY(Pow_6888x,MathIeeeSingTrans,15)(void);
extern void AROS_SLIB_ENTRY(Sqrt_6888x,MathIeeeSingTrans,16)(void);
extern void AROS_SLIB_ENTRY(Tieee_6888x,MathIeeeSingTrans,17)(void);
extern void AROS_SLIB_ENTRY(Fieee_6888x,MathIeeeSingTrans,18)(void);
extern void AROS_SLIB_ENTRY(Asin_6888x,MathIeeeSingTrans,19)(void);
extern void AROS_SLIB_ENTRY(Acos_6888x,MathIeeeSingTrans,20)(void);
extern void AROS_SLIB_ENTRY(Log10_6888x,MathIeeeSingTrans,21)(void);

struct Library * MathIeeeSingBasBase;

static int IEEESPT_Init(struct Library *lh)
{
    MathIeeeSingBasBase = OpenLibrary ("mathieeesingbas.library", 39);
    if (!MathIeeeSingBasBase)
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
	/* SetFunc(17, Tieee_6888x); */
	/* SetFunc(18, Fieee_6888x); */
	SetFunc(19, Asin_6888x);
	SetFunc(20, Acos_6888x);
	SetFunc(21, Log10_6888x);
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
