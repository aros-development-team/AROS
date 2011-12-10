/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#ifndef __MATHIEEESP_INTERN_H__
#define __MATHIEEESP_INTERN_H__

/* the following line is necessary so that the function headers are
   created correctly and the functions can be compiled properly */

#define float LONG

/* This is a short file that contains a few things every mathieeespbas
   function needs
*/
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif

#include <libraries/mathieeesp.h>
#include <aros/libcall.h>
#include <proto/mathieeesingbas.h>
#include <proto/exec.h>

/* Replace obsolete struct LibHeader with struct Library */
#define LibHeader Library

#define IEEESPMantisse_Mask 0x007FFFFF /* 23 bit for the mantisse  */
#define IEEESPExponent_Mask 0x7F800000 /*  8 bit for the exponent  */
#define IEEESPSign_Mask     0x80000000 /*  1 bit for the sign      */

#define Zero_Bit     0x00000004  /* Flags of the 680xx CPU         */
#define Negative_Bit 0x00000008  
#define Overflow_Bit 0x00000002  

#ifdef __mc68000
#include <aros/m68k/libcall_cc.h>
#endif

#endif /* __MATHFFP_INTERN_H__  */
