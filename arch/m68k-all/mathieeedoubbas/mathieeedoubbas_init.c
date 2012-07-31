/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id: mathieeedoubbas_init.c $

    Desc: Init of 68k/FPU mathieeedoubbas.library
    Lang: english
*/
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <utility/utility.h> /* this must be before mathieeedoubbas_intern.h */

#include "mathieeedoubbas_intern.h"

#define SetFunc(a,b) SetFunction((struct Library *)lh, a * -LIB_VECTSIZE, AROS_SLIB_ENTRY(b,MathIeeeDoubBas,a))

extern void AROS_SLIB_ENTRY(Fix_6888x,MathIeeeDoubBas,5)(void);
extern void AROS_SLIB_ENTRY(Flt_6888x,MathIeeeDoubBas,6)(void);

extern void AROS_SLIB_ENTRY(Abs_6888x,MathIeeeDoubBas,9)(void);
extern void AROS_SLIB_ENTRY(Neg_6888x,MathIeeeDoubBas,10)(void);
extern void AROS_SLIB_ENTRY(Add_6888x,MathIeeeDoubBas,11)(void);
extern void AROS_SLIB_ENTRY(Sub_6888x,MathIeeeDoubBas,12)(void);
extern void AROS_SLIB_ENTRY(Mul_6888x,MathIeeeDoubBas,13)(void);
extern void AROS_SLIB_ENTRY(Div_6888x,MathIeeeDoubBas,14)(void);
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

ADD2INITLIB(IEEEDP_Init, 0)
ADD2OPENLIB(IEEEDP_Open, 0)
