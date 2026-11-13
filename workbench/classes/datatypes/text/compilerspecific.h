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
#define VARARGS
#define __varargs68k

#define getreg(x) 0
#define putreg(a,b) 

#else

#ifdef __MORPHOS__

#define SAVEDS
#define ASM
#define STDARGS

#else

#define SAVEDS  __saveds
#define ASM     __asm
#define STDARGS __stdargs

typedef unsigned long IPTR;
#endif

#if !defined(_DOS_H) && defined(__SASC)
#include <dos.h>
#endif

#endif

#endif /* COMPILERSPECIFIC_H */
