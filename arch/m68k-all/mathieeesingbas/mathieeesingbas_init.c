/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Init of 68k/FPU mathieeesingbas.library
    Lang: english
*/
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <utility/utility.h> /* this must be before mathieeesingbas_intern.h */

#include "mathieeesingbas_intern.h"

#define SetFunc(a,b) SetFunction(lh, a * -LIB_VECTSIZE, AROS_SLIB_ENTRY(b,MathIeeeSingBas,a))

extern void AROS_SLIB_ENTRY(Fix_6888x,MathIeeeSingBas,5)(void);
extern void AROS_SLIB_ENTRY(Flt_6888x,MathIeeeSingBas,6)(void);

extern void AROS_SLIB_ENTRY(Abs_6888x,MathIeeeSingBas,9)(void);
extern void AROS_SLIB_ENTRY(Neg_6888x,MathIeeeSingBas,10)(void);
extern void AROS_SLIB_ENTRY(Add_6888x,MathIeeeSingBas,11)(void);
extern void AROS_SLIB_ENTRY(Sub_6888x,MathIeeeSingBas,12)(void);
extern void AROS_SLIB_ENTRY(Mul_6888x,MathIeeeSingBas,13)(void);
extern void AROS_SLIB_ENTRY(Div_6888x,MathIeeeSingBas,14)(void);
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
	SetFunc(5, Fix_6888x);
	SetFunc(6, Flt_6888x);
	//SetFunc(7, Cmp_6888x); use software
	//SetFunc(8, Tst_6888x); use software
	SetFunc(9, Abs_6888x);
	SetFunc(10, Neg_6888x);
	SetFunc(11, Add_6888x);
	SetFunc(12, Sub_6888x);
	SetFunc(13, Mul_6888x);
	SetFunc(14, Div_6888x);
	//SetFunc(15, Floor_6888x); use software
	//SetFunc(16, Ceil_6888x); use software
    }
    return TRUE;
}

ADD2INITLIB(IEEESP_Init, 0)
ADD2OPENLIB(IEEESP_Open, 0)
