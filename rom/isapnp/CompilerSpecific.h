#include <exec/types.h>

#ifdef __AROS__
#include <aros/debug.h>
#include <aros/libcall.h>
#else
#define bug KPrintF
#endif

#ifndef COMPILERSPECIFIC_H
#define COMPILERSPECIFIC_H
/*
**	$VER: CompilerSpecific.h 2.3 lcs (9.1.99)
**
**	Copyright (C) 1997 Bernardo Innocenti. All rights reserved.
**      Copyright (C) 2009-2013 The AROS Development Team
**
**	Compiler specific definitions is here. You can add support
**	for other compilers in this header. Please return any changes
**	you make to me, so I can add them to my personal copy of this file.
**
**	Here is a short description of the macros defined below:
**
**	LIBCALL
**		Shared library entry point, with register args
**
**	HOOKCALL
**		Hook or boopsi dispatcher entry point with arguments
**		passed in registers
**
**	INLINE
**		Please put function body inline to the calling code
**
**	STDARGS
**		Function uses standard C conventions for arguments
**
**	ASMCALL
**		Function takes arguments in the specified 68K registers
**
**	REGCALL
**		Function takes arguments in registers choosen by the compiler
**
**	CONSTCALL
**		Function does not modify any global variable
**
**	FORMATCALL(archetype,string_index,first_to_check)
**		Function uses printf or scanf-like formatting
**
**	SAVEDS
**		Function needs to reload context for small data model
**
**	INTERRUPT
**		Function will be called from within an interrupt
**
**	NORETURN
**		Function does never return
**
**	ALIGNED
**		Variable must be aligned to longword boundaries
**
**	CHIP
**		Variable must be stored in CHIP RAM
**
**	REG(reg,arg)
**		Put argument <arg> in 68K register <reg>
**
**	min(a,b)
**		Return the minimum between <a> and <b>
**
**	max(a,b)
**		Return the maximum between <a> and <b>
**
**	abs(a)
**		Return the absolute value of <a>
**
**	_COMPILED_WITH
**		A string containing the name of the compiler
*/

#ifdef __SASC
	/* SAS/C 6.58 or better */

	#define INLINE		static __inline
	#define STDARGS		__stdargs
	#define ASMCALL		__asm
	#define REGCALL		__regcall
	#define CONSTCALL	/* unsupported */
	#define FORMATCALL	/* unsupported */
	#define SAVEDS		__saveds
	#define INTERRUPT	__interrupt
	#define NORETURN	/* unsupported */
	#define ALIGNED		__aligned
	#define CHIP		__chip
	#define REG(reg,arg) register __##reg arg
	#define _COMPILED_WITH	"SAS/C"

	/* For min(), max() and abs() */
	#define USE_BUILTIN_MATH
	#include <string.h>
#else
#ifdef __GNUC__
	/* GeekGadgets GCC 2.7.2.1 or better */

	#define INLINE		static inline
	#define STDARGS		__attribute__((stkparm))
	#define ASMCALL		/* nothing */
	#define REGCALL		/* nothing */
	#define CONSTCALL	__attribute__((const))
	#define FORMATCALL(a,s,f)	__attribute__((format(a,s,f)))
	#define SAVEDS		__attribute__((saveds))
	#define INTERRUPT	__attribute__((interrupt))
	#define NORETURN	__attribute__((noreturn))
	#define ALIGNED		__attribute__((aligned(4)))
	#define CHIP		__attribute__((chip))
#ifdef m68000
	#define REG(reg,arg) arg __asm(#reg)
#else
	#define REG(reg,arg) arg
#endif
	#define _COMPILED_WITH	"GCC"

	#ifndef min
	#define min(a,b)	(((a)<(b))?(a):(b))
	#endif
	#ifndef max
	#define max(a,b)	(((a)>(b))?(a):(b))
	#endif
#ifndef __AROS__
	#ifndef abs
	#define abs(a)		(((a)>0)?(a):-(a))
	#endif
#endif

#if 0
	/* GCC produces code which calls these two functions
	 * to initialize and copy structures and arrays.
	 */
	static inline void __attribute__((stkparm)) bzero (char *buf, int len)
		{ while (len--) *buf++ = 0; }

	static inline void __attribute__((stkparm)) bcopy (char *src, char *dest, int len)
		{ while (len--) *dest++ = *src++; }
#endif

#else
#ifdef __STORM__
	/* StormC 2.00.23 or better */
	#define INLINE		__inline
	#define STDARGS		/* nothing */
	#define ASMCALL		/* nothing */
	#define REGCALL		register
	#define CONSTCALL	/* unsupported */
	#define FORMATCALL	/* unsupported */
	#define SAVEDS		__saveds
	#define INTERRUPT	__interrupt
	#define NORETURN	/* unsupported */
	#define ALIGNED		/* unsupported */
	#define CHIP		__chip
	#define REG(reg,arg) register __##reg arg
	#define _COMPILED_WITH	"StormC"

	#define min(a,b)	(((a)<(b))?(a):(b))
	#define max(a,b)	(((a)>(b))?(a):(b))
	#define abs(a)		(((a)>0)?(a):-(a))

	#define _INLINE_INCLUDES
	#include <string.h>
#else
#ifdef __MAXON__
	/* Maxon C/C++ */

	#define INLINE		static inline
	#define STDARGS		/* ? */
	#define ASMCALL		/* ? */
	#define REGCALL		/* ? */
	#define CONSTCALL	/* unsupported */
	#define FORMATCALL	/* unsupported */
	#define SAVEDS		__saveds
	#define INTERRUPT	__interrupt
	#define NORETURN	/* unsupported */
	#define ALIGNED		__aligned
	#define REG(reg,arg) register __##reg arg
	#define _COMPILED_WITH	"Maxon C"

	/* For min(), max() and abs() */
	#define USE_BUILTIN_MATH
	#include <string.h>

	#error Maxon C compiler support is untested. Please check all the above definitions
#else
#ifdef _DCC
	/* DICE C */

	#define INLINE		static __inline
	#define STDARGS		__stdargs
	#define ASMCALL		/* nothing */
	#define REGCALL		/* ? */
	#define CONSTCALL	/* unsupported */
	#define FORMATCALL	/* unsupported */
	#define SAVEDS		__geta4
	#define INTERRUPT	/* unsupported */
	#define NORETURN	/* unsupported */
	#define ALIGNED		__aligned
	#define REG(reg,arg)	__##reg arg
	#define _COMPILED_WITH	"DICE"

	#ifndef min
	#define min(a,b)	(((a)<(b))?(a):(b))
	#endif
	#ifndef max
	#define max(a,b)	(((a)>(b))?(a):(b))
	#endif
	#ifndef abs
	#define abs(a)		(((a)>0)?(a):-(a))
	#endif

	#error DICE compiler support is untested. Please check all the above definitions
#else
#ifdef AZTEC_C
	/* Aztec/Manx C */

	#define INLINE		static
	#define STDARGS		/* ? */
	#define ASMCALL		/* ? */
	#define REGCALL		/* ? */
	#define CONSTCALL	/* unsupported */
	#define FORMATCALL	/* unsupported */
	#define SAVEDS		__geta4
	#define INTERRUPT	/* unsupported */
	#define NORETURN	/* unsupported */
	#define ALIGNED		__aligned
	#define REG(reg,arg)	__##reg arg
	#define _COMPILED_WITH	"Manx C"

	#ifndef min
	#define min(a,b)	(((a)<(b))?(a):(b))
	#endif
	#ifndef max
	#define max(a,b)	(((a)>(b))?(a):(b))
	#endif
	#ifndef abs
	#define abs(a)		(((a)>0)?(a):-(a))
	#endif

	#error Aztec/Manx C compiler support is untested. Please check all the above definitions
#else
	#error Please add compiler specific definitions for your compiler
#endif
#endif
#endif
#endif
#endif
#endif


/* Special function attributes */

#define LIBCALL		ASMCALL SAVEDS
#define HOOKCALL	ASMCALL SAVEDS


/* AROS Compatibility: IPTR is a type which can store a pointer
 * as well as a long integer.
 */
#ifndef AROS_INTPTR_TYPE
#define AROS_INTPTR_TYPE long
typedef unsigned AROS_INTPTR_TYPE IPTR;
typedef signed   AROS_INTPTR_TYPE SIPTR;
#endif /* AROS_INTPTR_TYPE */



#endif /* !COMPILERSPECIFIC_H */
