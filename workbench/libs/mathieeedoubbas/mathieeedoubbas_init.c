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
#define SetFunc(a,b) SetFunction((struct Library *)lh, a * -LIB_VECTSIZE, AROS_SLIB_ENTRY(b,MathIeeeDoubBas))

extern QUAD MathIeeeDoubBas_FPU_IEEEDPFix(); 
extern QUAD MathIeeeDoubBas_FPU_IEEEDPFlt();
extern QUAD MathIeeeDoubBas_FPU_IEEEDPMul(); 
extern QUAD MathIeeeDoubBas_FPU_IEEEDPDiv();

AROS_SET_LIBFUNC(IEEEDP_Init, LIBBASETYPE, lh)
{
    AROS_SET_LIBFUNC_INIT
/*
    SetFunc( 6, FPU_IEEEDPFlt);
*/
    SetFunc(13, FPU_IEEEDPMul);
    SetFunc(14, FPU_IEEEDPDiv);
     
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
} /* L_InitLib */

ADD2INITLIB(IEEEDP_Init, 0)
