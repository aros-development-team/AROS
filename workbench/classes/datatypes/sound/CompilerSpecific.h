#ifndef COMPILERSPECIFIC_H
#define COMPILERSPECIFIC_H
/*
**	$VER: CompilerSpecific.h 2.2 (1.10.97)
**
**	Copyright (C) 1997 Bernardo Innocenti. All rights reserved.
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
	#define ASMCALL		/* nothing */
	#define REGCALL		/* nothing */
	#define CONSTCALL	__attribute__((const))
	#define FORMATCALL(a,s,f)	__attribute__((format(a,s,f)))
	#define NORETURN	__attribute__((noreturn))
#ifdef __AROS__
	#define STDARGS
    	#define SAVEDS
	#define INTERRUPT
	#define ALIGNED
	#define REG(reg,arg) arg
	
	#define __regargs
	#define __interrupt
	
#else
	#define STDARGS		__attribute__((stkparm))
	#define SAVEDS		__attribute__((saveds))
	#define INTERRUPT	__attribute__((interrupt))
	#define ALIGNED		__attribute__((aligned(4)))
	#define REG(reg,arg) arg __asm(#reg)
#endif

	#define _COMPILED_WITH	"GCC"

	#define min(a,b)	(((a)<(b))?(a):(b))
	#define max(a,b)	(((a)>(b))?(a):(b))
	#define abs(a)		(((a)>0)?(a):-(a))

	/* GCC produces code which calls these two functions
	 * to initialize and copy structures and arrays.
	 */
	void INLINE STDARGS bzero (char *buf, int len)
		{ while (len--) *buf++ = 0; }

	void INLINE STDARGS bcopy (char *src, char *dest, int len)
		{ while (len--) *dest++ = *src++; }

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

	#define min(a,b)	(((a)<(b))?(a):(b))
	#define max(a,b)	(((a)>(b))?(a):(b))
	#define abs(a)		(((a)>0)?(a):-(a))

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

	#define min(a,b)	(((a)<(b))?(a):(b))
	#define max(a,b)	(((a)>(b))?(a):(b))
	#define abs(a)		(((a)>0)?(a):-(a))

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
#ifndef IPTR
#define IPTR LONG
#endif /* IPTR */



#endif /* !COMPILERSPECIFIC_H */
