/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Init of mathieeedoubbas.library
    Lang: english
*/
#include <utility/utility.h> /* this must be before mathieeedoubbas_intern.h */

#include <aros/symbolsets.h>

#include "mathieeedoubbas_intern.h"
#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/* */
#define SetFunc(a,b) SetFunction((struct Library *)lh, (LONG)(a * -LIB_VECTSIZE), AROS_SLIB_ENTRY(b,MathIeeeDoubBas,a))

extern QUAD AROS_SLIB_ENTRY(FPU_IEEEDPFix,MathIeeeDoubBas,5)(); 
extern QUAD AROS_SLIB_ENTRY(FPU_IEEEDPFlt,MathIeeeDoubBas,6)();
extern QUAD AROS_SLIB_ENTRY(FPU_IEEEDPMul,MathIeeeDoubBas,13)(); 
extern QUAD AROS_SLIB_ENTRY(FPU_IEEEDPDiv,MathIeeeDoubBas,14)();

static int IEEEDP_Init(LIBBASETYPEPTR lh)
{
/*
    SetFunc( 6, FPU_IEEEDPFlt);
*/
    SetFunc(13, FPU_IEEEDPMul);
    SetFunc(14, FPU_IEEEDPDiv);
     
    return TRUE;
} /* L_InitLib */

ADD2INITLIB(IEEEDP_Init, 0)
