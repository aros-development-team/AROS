/*
    Copyright (C) 1995-1998 AROS - The Amiga Replacement OS
    *** Automatic generated file. Do not edit ***
    Desc: Function table for BOOPSI
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
extern void AROS_SLIB_ENTRY(AddClass,BASENAME) (void);
extern void AROS_SLIB_ENTRY(DisposeObject,BASENAME) (void);
extern void AROS_SLIB_ENTRY(FindClass,BASENAME) (void);
extern void AROS_SLIB_ENTRY(FreeClass,BASENAME) (void);
extern void AROS_SLIB_ENTRY(GetAttr,BASENAME) (void);
extern void AROS_SLIB_ENTRY(MakeClass,BASENAME) (void);
extern void AROS_SLIB_ENTRY(NewObjectA,BASENAME) (void);
extern void AROS_SLIB_ENTRY(NextObject,BASENAME) (void);
extern void AROS_SLIB_ENTRY(RemoveClass,BASENAME) (void);
extern void AROS_SLIB_ENTRY(SetAttrsA,BASENAME) (void);
extern void AROS_SLIB_ENTRY(FreeICData,BASENAME) (void);
extern void AROS_SLIB_ENTRY(DoNotify,BASENAME) (void);

void *const LIBFUNCTABLE[]=
{
    AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader),
    AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader),
    AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader),
    AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader),
    AROS_SLIB_ENTRY(AddClass,BASENAME), /* 5 */
    AROS_SLIB_ENTRY(DisposeObject,BASENAME), /* 6 */
    AROS_SLIB_ENTRY(FindClass,BASENAME), /* 7 */
    AROS_SLIB_ENTRY(FreeClass,BASENAME), /* 8 */
    AROS_SLIB_ENTRY(GetAttr,BASENAME), /* 9 */
    AROS_SLIB_ENTRY(MakeClass,BASENAME), /* 10 */
    AROS_SLIB_ENTRY(NewObjectA,BASENAME), /* 11 */
    AROS_SLIB_ENTRY(NextObject,BASENAME), /* 12 */
    AROS_SLIB_ENTRY(RemoveClass,BASENAME), /* 13 */
    AROS_SLIB_ENTRY(SetAttrsA,BASENAME), /* 14 */
    AROS_SLIB_ENTRY(FreeICData,BASENAME), /* 15 */
    AROS_SLIB_ENTRY(DoNotify,BASENAME), /* 16 */
    (void *)-1L
};
