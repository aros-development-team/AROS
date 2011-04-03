/*
    Copyright © 1995-96, The AROS Development Team. All rights reserved.
    *** Automatic generated file. Do not edit ***
    Desc: Function table for Example
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
extern void AROS_SLIB_ENTRY(EXF_TestRequest,BASENAME) (void);

void *const LIBFUNCTABLE[]=
{
    AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader),
    AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader),
    AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader),
    AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader),
    AROS_SLIB_ENTRY(EXF_TestRequest,BASENAME), /* 5 */
    (void *)-1L
};
