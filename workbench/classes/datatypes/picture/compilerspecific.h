#ifndef COMPILERSPECIFIC_H
#define COMPILERSPECIFIC_H

#undef SAVEDS
#undef ASM
#undef STDARGS

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifdef _AROS

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

typedef unsigned long IPTR;

#define SAVEDS  __saveds
#define ASM     __asm
#define STDARGS __stdargs

#if !defined(_DOS_H) && defined(__SASC)
#include <dos.h>
#endif

#endif

#endif /* COMPILERSPECIFIC_H */
