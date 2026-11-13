#ifndef DEBUG_H
#define DEBUG_H
/*
**	$VER: Debug.h 2.1 (19.10.97)
**
**	Copyright (C) 1995,96,97 Bernardo Innocenti. All rights reserved.
**
**	Use 4 chars wide TABs to read this file
**
**	Some handy debug macros which are automatically excluded when the
**	DEBUG preprocessor sysmbol is not defined. To make debug executables,
**	link with debug.lib or any module containing the kprintf() function.
**
**	Here is a short description of the macros defined below:
**
**	ILLEGAL
**		Output an inline "ILLEGAL" 68K opcode, which will
**		be interpreted as a breakpoint by most debuggers.
**
**	DBPRINTF
**		Output a formatted string to the debug console. This
**		macro uses the debug.lib kprintf() function by default.
**
**	ASSERT(x)
**		Do nothing if the expression <x> evalutates to a
**		non-zero value, output a debug message otherwise.
**
**	ASSERT_VALID(x)
**		Checks if the expression <x> points to a valid
**		memory location, and outputs a debug message
**		otherwise. A NULL pointer is considered VALID.
**
**	ASSERT_VALIDNO0(x)
**		Checks if the expression <x> points to a valid
**		memory location, and outputs a debug message
**		otherwise. A NULL pointer is considered INVALID.
**
**	DB(x)
**		Compile the expression <x> when making a debug
**		executable, leave it out otherwise.
*/

#ifdef DEBUG_ME

	/* Needed for TypeOfMem() */
	#ifndef  PROTO_EXEC_H
	#include <proto/exec.h>
	#endif /* PROTO_EXEC_H */

	#if defined(__SASC)

		extern void __builtin_emit (int);
		#define ILLEGAL __builtin_emit(0x4AFC)
		STDARGS extern void kprintf (const char *, ...);

	#elif defined(__GNUC__)

		/* Currently, there is no kprintf() function in libamiga.a */
		#define kprintf printf

		/* GCC doesn't accept asm statemnts in the middle of an
		 * expression such as "a ? b : asm(something)".
		 */
		#define ILLEGAL illegal()
		static inline int illegal(void) { asm ("illegal"); return 0; }
		extern void STDARGS FORMATCALL(printf,1,2) kprintf (const char *, ...);

	#else
		#error Please add compiler specific definitions for your compiler
	#endif

	#if defined(__SASC) || defined (__GNUC__)

		/* common definitions for ASSERT and DB macros */

		#define DBPRINTF kprintf

		#define ASSERT(x) ( (x) ? 0 :										\
			( DBPRINTF ("\x07%s, %ld: assertion failed: " #x "\n",			\
			__FILE__, __LINE__) , ILLEGAL ) );

		#define ASSERT_VALID(x) ( ((((APTR)(x)) == NULL) ||					\
			(((LONG)(x) > 1024) &&	TypeOfMem ((APTR)(x)))) ? 0 :			\
			( DBPRINTF ("\x07%s, %ld: bad address: " #x " = $%lx\n",		\
			__FILE__, __LINE__, (APTR)(x)) , ILLEGAL ) );

		#define ASSERT_VALIDNO0(x) ( (((LONG)(x) > 1024) &&					\
			TypeOfMem ((APTR)(x))) ? 0 :									\
			( DBPRINTF ("\x07%s, %ld: bad address: " #x " = $%lx\n",		\
			__FILE__, __LINE__, (APTR)(x)) , ILLEGAL ) );

		#define DB(x) x
	#endif

#else
	#define ASSERT_VALID(x)
	#define ASSERT_VALIDNO0(x)
	#define ASSERT(x)
	#define DB(x)
#endif /* DEBUG */

#endif /* !DEBUG_H */
