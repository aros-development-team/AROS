/*
**      $VER: compiler.h 37.31 (18.3.98)
**
**      Compiler independent register (and SAS/C extensions) handling
**
**      (C) Copyright 1997-98 Andreas R. Kleinert
**      All Rights Reserved.
*/

#ifndef COMPILER_H
#define COMPILER_H

 /* ============================================================
    There have been problems how to define the seglist pointer
    under AROS, AmigaOS or elsewhere in a unique (and still
    working!) way. It seems to make sense to use a new, global
    type definition for it. This is done here. */

#ifndef _AROS
# ifdef VBCC
#  define SEGLISTPTR APTR
# else /* !VBCC */
#  include <dos/dos.h>
#  define SEGLISTPTR BPTR
# endif /* VBCC */
#else /* !_AROS */
  typedef struct SegList * SEGLISTPTR;
#endif /* _AROS */

 /* ============================================================ */

/* Basically, Amiga C compilers must try to reach the goal to be
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
/* Method 1: redefining SAS/C keywords                                   */
/*                                                                       */
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

#ifdef __GNUC__  // ignore this switches of SAS/Storm
#define __d0
#define __d1
#define __d2
#define __d3
#define __d4
#define __d5
#define __d6
#define __d7
#define __a0
#define __a1
#define __a2
#define __a3
#define __a4
#define __a5
#define __a6
#define __a7
#endif

#ifdef VBCC
#define __d0 __reg("d0")
#define __d1 __reg("d1")
#define __d2 __reg("d2")
#define __d3 __reg("d3")
#define __d4 __reg("d4")
#define __d5 __reg("d5")
#define __d6 __reg("d6")
#define __d7 __reg("d7")
#define __a0 __reg("a0")
#define __a1 __reg("a1")
#define __a2 __reg("a2")
#define __a3 __reg("a3")
#define __a4 __reg("a4")
#define __a5 __reg("a5")
#define __a6 __reg("a6")
#define __a7 __reg("a7")
#endif

 /* for SAS/C we don't need this, for StormC this is done in the
    makefile or projectfile */

/*                                                                       */
/* ********************************************************************* */


/* ********************************************************************* */
/* Method 2: defining our own keywords                                   */
/*                                                                       */
#ifdef __SASC

#  define REG(r)     register __ ## r
#  define GNUCREG(r)
#  define SAVEDS     __saveds
#  define ASM        __asm
#  define REGARGS    __regargs
#  define STDARGS    __stdargs
#  define ALIGNED    __aligned

#else
# ifdef __MAXON__

#  define REG(r)    register __ ## r
#  define GNUCREG(r)
#  define SAVEDS
#  define ASM
#  define REGARGS
#  define STDARGS
#  define ALIGNED

# else
#  ifdef __STORM__

#   define REG(r)  register __ ## r
#   define GNUCREG(r)
#   define SAVEDS  __saveds
#   define ASM
#   define REGARGS
#   define STDARGS
#   define ALIGNED

#  else
#   ifdef __GNUC__

#    ifdef __AROS__
#     define REG(r)
#     define GNUCREG(r)
#     define SAVEDS
#     define ASM
#     define REGARGS
#     define STDARGS
#     define ALIGNED
#     define register
#     define __saveds
#     define __asm
#     define chip
#    else
#     define REG(r)
#     define GNUCREG(r)  __asm( #r )
#     define SAVEDS  __saveds
#     define ASM
#     define REGARGS __regargs
#     define STDARGS __stdargs
#     define ALIGNED __aligned
#    endif

#   else
#    ifdef VBCC
/* VBCC ignore this switch */
#     define __aligned
#     define __asm
#     define __regargs
#     define __saveds
#     define __stdargs
#     define __register
#     define GNUCREG(r)
#     define REG(r)
#     define SAVEDS
#     define ASM
#     define REGARGS
#     define STDARGS
#     define ALIGNED

#    else
#     ifdef _DCC
#      define __aligned
#      define __stdargs
#      define GNUCREG(r)
#      define ASM

#     else  /* any other compiler, to be added here */

#      define REG(r)
#      define GNUCREG(r)
#      define SAVEDS
#      define ASM
#      define REGARGS
#      define STDARGS
#      define ALIGNED

#     endif /*   _DCC      */
#    endif /*   VBCC      */
#   endif /*  __GNUC__   */
#  endif /*  __STORM__  */
# endif /*  __MAXON__  */
#endif /*  __SASC     */
/*                                                                       */
/* ********************************************************************* */

#endif /* COMPILER_H */
