#ifndef LIBCORE_COMPILER_H
#define LIBCORE_COMPILER_H
/*
**	$VER: compiler.h 37.15 (14.8.97)
**
**	Compiler independent register (and SAS/C extensions) handling
**
**	(C) Copyright 1997 Andreas R. Kleinert
**	All Rights Reserved.
*/
/* This might define _AROS */
#ifndef EXEC_TYPE_H
#   include <exec/types.h>
#endif

/* Basically, Amiga C compilers must reach the goal to be
   as SAS/C compatible as possible. But on the other hand,
   when porting AmigaOS to other platforms, one perhaps
   can't expect GCC becoming fully SAS/C compatible...

   There are two ways to make your sources portable:

    - using non ANSI SAS/C statements and making these
      "available" to the other compilers (re- or undefining)
    - using replacements for SAS/C statements and smartly
      redefining these for any compiler

   The last mentioned is the most elegant, but may require to
   rewrite your source codes, so this compiler include file
   basically does offer both.

   For some compilers, this may have been done fromout project or
   makefiles for the first method (e.g. StormC) to ensure compileablity.

   Basically, you should include this header file BEFORE any other stuff.
*/

/* ********************************************************************* */
/* Method 1: redefining SAS/C keywords					 */
/*									 */
/* Sorry, this method does not work with register definitions for the current
gcc version (V2.7.2.1), as it expects register attributes after the parameter
description. (This is announced to be fixed with gcc V2.8.0).
Moreover the __asm keyword has another meaning with GCC.
Therefore ASM must be used. */

#ifdef __MAXON__  // ignore this switches of SAS/Storm
#define __aligned
#define __asm
#define __regargs
#define __saveds
#define __stdargs
#endif


 /* for SAS/C we don't need this, for StormC this is done in the
    makefile or projectfile */

/*									 */
/* ********************************************************************* */


/* ********************************************************************* */
/* Method 2: defining our own keywords					 */
/*									 */
#ifdef __SASC

#  define REG(r)     register __ ## r
#  define GNUCREG(r)
#  define SAVEDS     __saveds
#  define ASM	     __asm
#  define REGARGS    __regargs
#  define STDARGS    __stdargs
#  define ALIGNED    __aligned

#else
# ifdef __MAXON__

#   define REG(r)    register __ ## r
#   define GNUCREG(r)
#   define SAVEDS
#   define ASM
#   define REGARGS
#   define STDARGS
#   define ALIGNED

# else
#   ifdef __STORM__

#     define REG(r)  register __ ## r
#     define GNUCREG(r)
#     define SAVEDS  __saveds
#     define ASM
#     define REGARGS
#     define STDARGS
#     define ALIGNED

#   else
#     ifdef __GNUC__

#	ifndef _AROS /* No AROS ? */

#	    define REG(r)
#	    define GNUCREG(r)   __asm( ## r)
#	    define SAVEDS	__saveds
#	    define ASM
#	    define REGARGS	__regargs
#	    define STDARGS	__stdargs
#	    define ALIGNED	__aligned

#	else /* _AROS */

#	    define REG(r)
#	    define GNUCREG(r)
#	    define SAVEDS
#	    define ASM
#	    define REGARGS
#	    define STDARGS
#	    define ALIGNED

#	endif /* _AROS */

#     else /* any other compiler, to be added here */

#	define REG(r)
#	define GNUCREG(r)
#	define SAVEDS
#	define ASM
#	define REGARGS
#	define STDARGS
#	define ALIGNED

#     endif /* __GNUC__ */
#   endif /* __STORM__ */
# endif /* __MAXON__ */
#endif /* __SASC */

#ifdef _AROS
#   include <aros/libcall.h>
#else
#   define D0 d0
#   define D1 d1
#   define D2 d2
#   define D3 d3
#   define D4 d4
#   define D5 d5
#   define D6 d6
#   define D7 d7
#   define A0 a0
#   define A1 a1
#   define A2 a2
#   define A3 a3
#   define A4 a4
#   define A5 a5
#   define A6 a6
#   define A7 a7

#   define _AROS_LHA(t,n,r) REG(r) t n GNUCREG (# r)
#   define AROS_LHA(t,n,r)  _AROS_LHA(t,n,r)

#   define AROS_LH0(rettype,name,libBaseType,libBase,offset,libName) \
    rettype SAVEDS ASM name (\
	AROS_LHA(libBaseType,libBase,A6) \
    )

#   define AROS_LH1(rettype,name,a1,libBaseType,libBase,offset,libName) \
    rettype SAVEDS ASM name (\
	a1, \
	AROS_LHA(libBaseType,libBase,A6) \
    )

#   define AROS_LH2(rettype,name,a1,a2,libBaseType,libBase,offset,libName) \
    rettype SAVEDS ASM name (\
	a1, \
	a2, \
	AROS_LHA(libBaseType,libBase,A6) \
    )

#   define AROS_LH3(rettype,name,a1,a2,a3,libBaseType,libBase,offset,libName) \
    rettype SAVEDS ASM name (\
	a1, \
	a2, \
	a3, \
	AROS_LHA(libBaseType,libBase,A6) \
    )

#   define AROS_LP3 AROS_LH3

#   define AROS_SLIB_ENTRY(n,s)     n

#endif


/*									 */
/* ********************************************************************* */
#endif /* LIBCORE_COMPILER_H */
