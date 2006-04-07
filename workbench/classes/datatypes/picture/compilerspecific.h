/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef COMPILERSPECIFIC_H
#define COMPILERSPECIFIC_H

#undef SAVEDS
#undef ASM
#undef STDARGS

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifdef __AROS__

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif

#define SAVEDS
#define ASM
#define STDARGS

#define getreg(x) 0
#define putreg(a,b) 

#else

#ifdef __MORPHOS__
typedef unsigned long IPTR;
#endif

#define SAVEDS  __saveds
#define ASM     __asm
#define STDARGS __stdargs

#if !defined(_DOS_H) && defined(__SASC)
#include <dos.h>
#endif

#endif

#endif /* COMPILERSPECIFIC_H */
