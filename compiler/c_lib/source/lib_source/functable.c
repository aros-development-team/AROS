/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    *** Automatic generated file. Do not edit ***
    Desc: Function table for 
    Lang: english
*/
#ifndef LIBCORE_COMPILER_H
#   include <libcore/compiler.h>
#endif
#ifndef NULL
#define NULL ((void *)0)
#endif

#include "libdefs.h"
extern void AROS_SLIB_ENTRY(OpenLib,LibHeader) (void);
extern void AROS_SLIB_ENTRY(CloseLib,LibHeader) (void);
extern void AROS_SLIB_ENTRY(ExpungeLib,LibHeader) (void);
extern void AROS_SLIB_ENTRY(ExtFuncLib,LibHeader) (void);
extern void AROS_SLIB_ENTRY(EXF_TestRequest,BASENAME) (void);

void *const LIBFUNCTABLE[]=
{
    AROS_SLIB_ENTRY(OpenLib,LibHeader),
    AROS_SLIB_ENTRY(CloseLib,LibHeader),
    AROS_SLIB_ENTRY(ExpungeLib,LibHeader),
    AROS_SLIB_ENTRY(ExtFuncLib,LibHeader),
    AROS_SLIB_ENTRY(EXF_TestRequest,BASENAME), /* 5 */
    (void *)-1L
};
