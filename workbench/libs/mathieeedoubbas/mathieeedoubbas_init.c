/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Init of mathieeedoubbas.library
    Lang: english
*/
#include <utility/utility.h> /* this must be before mathieeedoubbas_intern.h */

#include "mathieeedoubbas_intern.h"
#include LC_LIBDEFS_FILE

#define LC_NO_OPENLIB
#define LC_NO_EXPUNGELIB
#define LC_NO_CLOSELIB
#define LC_RESIDENTPRI	    -120

#include <libcore/libheader.c>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/* */
#define SetFunc(a,b) SetFunction((struct Library *)lh, a * -LIB_VECTSIZE, AROS_SLIB_ENTRY(b,MathIeeeDoubBas))

extern QUAD MathIeeeDoubBas_FPU_IEEEDPFix(); 
extern QUAD MathIeeeDoubBas_FPU_IEEEDPFlt();
extern QUAD MathIeeeDoubBas_FPU_IEEEDPMul(); 
extern QUAD MathIeeeDoubBas_FPU_IEEEDPDiv();

struct ExecBase * SysBase; /* global variable */

ULONG SAVEDS L_InitLib (LC_LIBHEADERTYPEPTR lh)
{
    SysBase = lh->lh_SysBase;

/*
    SetFunc( 6, FPU_IEEEDPFlt);
*/
    SetFunc(13, FPU_IEEEDPMul);
    SetFunc(14, FPU_IEEEDPDiv);
     
    return TRUE;
} /* L_InitLib */

