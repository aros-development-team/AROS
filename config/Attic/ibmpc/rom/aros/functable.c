/*
    Copyright (C) 1995-1998 AROS - The Amiga Replacement OS
    *** Automatic generated file. Do not edit ***
    Desc: Function table for Aros
    Lang: english
*/
#ifndef LIBCORE_COMPILER_H
#   include <libcore/compiler.h>
#endif
#ifndef NULL
#define NULL ((void *)0)
#endif

#include "libdefs.h"
extern void AROS_SLIB_ENTRY(open, BASENAME) (void);
extern void AROS_SLIB_ENTRY(close, BASENAME) (void);
extern void AROS_SLIB_ENTRY(expunge, BASENAME) (void);
extern void AROS_SLIB_ENTRY(null, BASENAME) (void);
extern void AROS_SLIB_ENTRY(ArosInquireA,BASENAME) (void);

void *const LIBFUNCTABLE[]=
{
    AROS_SLIB_ENTRY(open, BASENAME),
    AROS_SLIB_ENTRY(close, BASENAME),
    AROS_SLIB_ENTRY(expunge, BASENAME),
    AROS_SLIB_ENTRY(null, BASENAME),
    AROS_SLIB_ENTRY(ArosInquireA,BASENAME), /* 5 */
    (void *)-1L
};
