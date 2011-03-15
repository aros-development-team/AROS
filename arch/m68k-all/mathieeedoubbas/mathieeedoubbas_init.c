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

#define SetFunc(a,b) SetFunction((struct Library *)lh, a * -LIB_VECTSIZE, AROS_SLIB_ENTRY(b,MathIeeeDoubBas))

extern void AROS_SLIB_ENTRY(Mul_6888x,MathIeeeDoubBas)(void);
extern void AROS_SLIB_ENTRY(Div_6888x,MathIeeeDoubBas)(void);

static int IEEEDP_Init(struct MathIeeeDoubBasBase *lh)
{
    if (SysBase->AttnFlags & (AFF_68881 | AFF_68040)) {
	SetFunc(13, Mul_6888x);
	SetFunc(14, Div_6888x);
    }
    return TRUE;
}

ADD2INITLIB(IEEEDP_Init, 0)
