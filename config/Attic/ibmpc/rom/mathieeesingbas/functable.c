/*
    Copyright (C) 1995-1998 AROS - The Amiga Replacement OS
    *** Automatic generated file. Do not edit ***
    Desc: Function table for Mathieeesingbas
    Lang: english
*/
#ifndef LIBCORE_COMPILER_H
#   include <libcore/compiler.h>
#endif
#ifndef NULL
#define NULL ((void *)0)
#endif

#include "libdefs.h"
extern void AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader) (void);
extern void AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader) (void);
extern void AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader) (void);
extern void AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader) (void);
extern void AROS_SLIB_ENTRY(IEEESPFix,BASENAME) (void);
extern void AROS_SLIB_ENTRY(IEEESPFlt,BASENAME) (void);
extern void AROS_SLIB_ENTRY(IEEESPCmp,BASENAME) (void);
extern void AROS_SLIB_ENTRY(IEEESPTst,BASENAME) (void);
extern void AROS_SLIB_ENTRY(IEEESPAbs,BASENAME) (void);
extern void AROS_SLIB_ENTRY(IEEESPNeg,BASENAME) (void);
extern void AROS_SLIB_ENTRY(IEEESPAdd,BASENAME) (void);
extern void AROS_SLIB_ENTRY(IEEESPSub,BASENAME) (void);
extern void AROS_SLIB_ENTRY(IEEESPMul,BASENAME) (void);
extern void AROS_SLIB_ENTRY(IEEESPDiv,BASENAME) (void);
extern void AROS_SLIB_ENTRY(IEEESPFloor,BASENAME) (void);
extern void AROS_SLIB_ENTRY(IEEESPCeil,BASENAME) (void);

void *const LIBFUNCTABLE[]=
{
    AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader),
    AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader),
    AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader),
    AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader),
    AROS_SLIB_ENTRY(IEEESPFix,BASENAME), /* 5 */
    AROS_SLIB_ENTRY(IEEESPFlt,BASENAME), /* 6 */
    AROS_SLIB_ENTRY(IEEESPCmp,BASENAME), /* 7 */
    AROS_SLIB_ENTRY(IEEESPTst,BASENAME), /* 8 */
    AROS_SLIB_ENTRY(IEEESPAbs,BASENAME), /* 9 */
    AROS_SLIB_ENTRY(IEEESPNeg,BASENAME), /* 10 */
    AROS_SLIB_ENTRY(IEEESPAdd,BASENAME), /* 11 */
    AROS_SLIB_ENTRY(IEEESPSub,BASENAME), /* 12 */
    AROS_SLIB_ENTRY(IEEESPMul,BASENAME), /* 13 */
    AROS_SLIB_ENTRY(IEEESPDiv,BASENAME), /* 14 */
    AROS_SLIB_ENTRY(IEEESPFloor,BASENAME), /* 15 */
    AROS_SLIB_ENTRY(IEEESPCeil,BASENAME), /* 16 */
    (void *)-1L
};
