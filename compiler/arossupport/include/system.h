#ifndef AROS_SYSTEM_H
#define AROS_SYSTEM_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Analyse the current kind of system and compiler.
    Lang: english
*/

#ifndef AROS_CPU_H
#   include <aros/cpu.h>
#endif

/**************************************
		Defines
**************************************/
/* 1. Analyze system: Specify a definitive define for each system */
#if defined(_AMIGA) || defined(AMIGA)
#   ifndef _AMIGA 
#	define _AMIGA
#   endif
#   ifndef AMIGA
#       define AMIGA
#   endif
#endif

#if defined __GNUC__ && defined __GNUC_MINOR__
#    define __GNUC_PREREQ(maj, min) \
         ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#    define __GNUC_PREREQ(maj, min) 0
#endif


/*
 * 2. Analyze compiler for STD C/C++.
 *
 * We test for the following:
 * a.	extern "C" linkage required for programs.
 * b.	inline, const, volatile, restrict keywords defined in
 *	newer C/C++ standards.
 * c.	throw() being available, which lets the compiler better
 *      optimize stuff
 */

#if defined(__cplusplus)
#   define __EXTERN extern "C"
#   define __BEGIN_DECLS    extern "C" {
#   define __BEGIN_EXTERN   extern "C" {
#   define __END_DECLS      };
#   define __END_EXTERN	    };
#else
#   define __EXTERN extern
#   define __BEGIN_DECLS
#   define __BEGIN_EXTERN
#   define __END_DECLS
#   define __END_EXTERN
#endif

#if defined(__STDC__) || defined(__cplusplus)
#   define	    __const__	    const
#   define	    __inline__	    inline
#   define	    __volatile__    volatile

/*
 * C99 defines a new keyword restrict that can help do optimisation where
 * pointers are used in programs. We'd like to support optimisation :-)
 */
#   if defined(__STDC__VERSION__) &&  __STDC__VERSION__ >= 199901L
#	define	    __restrict__    restrict
#   else
#	define	    __restrict__
#	define	    restrict
#   endif

#else
#   define	    __const__
#   define	    const
#   define	    __inline__
#   define	    inline
#   define	    __volatile__
#   define	    volatile
#   define	    __restrict__
#   define	    restrict
#endif

#ifdef __GNUC__
#    if defined __cplusplus && __GNUC_PREREQ (2,8)
#        define __THROW       throw ()
#    else
#        define __THROW
#    endif
#endif

/* 3. Macros for making things more efficient */
#if __GNUC_PREREQ(2,5)
#   define __noreturn  __attribute__((__noreturn__))
#else
#   define __noreturn
#endif

#if __GNUC_PREREQ(2,5) && !(__GNUC_PREREQ(2,6) && defined __cplusplus)
#   define __noeffect  __attribute__((__const__))
#else
#   define __noeffect
#endif

#if __GNUC_PREREQ(2,7)
#    define __unused   __attribute__((__unused__))
#else
#    define __unused
#endif

#if __GNUC_PREREQ(3,3)
#    define __used   __attribute__((__used__))
#else
#    define __used __unused
#endif

#if __GNUC_PREREQ(2,96)
#    define __pure     __attribute__((__pure__))
#else
#    define __pure
#endif

#if __GNUC_PREREQ(2,5)
#    define __const     __attribute__((__const__))
#else
#    define __const
#endif

/* 4. Macros for debugging and development */
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || (defined(__STDC__) && __STDC_VERSION__ >= 199901L)
#   define AROS_64BIT_TYPE long long
#   define AROS_HAVE_LONG_LONG
#endif

#if defined(__INTEL_COMPILER)
#   ifdef inline
#       undef inline
#   endif
#   define inline
#endif

#if __GNUC__ <= 2
#   define __deprecated
#endif
#if __GNUC__ > 2
#   define __deprecated    __attribute__((__deprecated__))
#endif


/* 5. Calculated #defines */
#if !AROS_STACK_GROWS_DOWNWARDS
#   define AROS_SLOWSTACKTAGS
#   define AROS_SLOWSTACKMETHODS
#endif /* !AROS_STACK_GROWS_DOWNWARDS */

#if !defined(__CONCAT)
#   if defined(__STDC__) || defined(__cplusplus)
#       define      __CONCAT1(a,b)  a ## b
#       define      __CONCAT(a,b)   __CONCAT1(a,b)
#   else
#       define      __CONCAT(a,b)       a/**/b
#   endif
#endif

#ifndef AROS_ASMSYMNAME
#   define AROS_ASMSYMNAME(n) n
#endif

#ifndef AROS_CSYM_FROM_ASM_NAME
#   ifdef __ELF__
#       define AROS_CSYM_FROM_ASM_NAME(n) n
#   else
#       error define AROS_CSYM_FROM_ASM_NAME for your architecture
#   endif
#endif

#define ___AROS_STR(x) #x
#define __AROS_STR(x) ___AROS_STR(x)


/* Makes a 'new' symbol which occupies the same memory location as the 'old' symbol */
#if !defined AROS_MAKE_ALIAS
#   define AROS_MAKE_ALIAS(old, new) \
        AROS_MAKE_ASM_SYM(typeof(old), new, AROS_CSYM_FROM_ASM_NAME(new), AROS_CSYM_FROM_ASM_NAME(old)); \
	AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(new))
#endif

/* define an asm symbol 'asym' with a C name 'csym', type 'type' and with value 'value'.
   'value' has to be an asm constant, thus either an address number or an asm symbol name, */
#if !defined AROS_MAKE_ASM_SYM 
#    define AROS_MAKE_ASM_SYM(type, csym, asym, value)             \
         extern type csym asm(__AROS_STR(asym));                   \
	 typedef int __CONCAT(__you_must_first_make_asym_, asym);  \
         asm(".set " __AROS_STR(asym) ", " __AROS_STR(value) "\n")
#endif

/* Makes an ASM symbol 'asym' available for use in the compilation unit this
   macro is used, with a C name 'csym'. This has also the side effect of
   triggering the inclusion, by the linker, of all code and data present in the
   module where the ASM symbol is actually defined.  */
#if !defined AROS_IMPORT_ASM_SYM 
#    define AROS_IMPORT_ASM_SYM(type, csym, asym)     \
         extern type csym asm(__AROS_STR(asym));      \
         asm("\n.globl " __AROS_STR(asym) "\n") 
#endif

/* Make sure other compilation units can see the symbol 'asym' created with AROS_MAKE_ASM_SYM.
   This macro results in a compile-time error in case it's used BEFORE the symbol has been
   made with AROS_MAKE_ASM_SYM.  */
#if !defined AROS_EXPORT_ASM_SYM 
#    define AROS_EXPORT_ASM_SYM(asym)                                    \
         struct __CONCAT(___you_must_first_make_asym_, asym)             \
	 {                                                               \
	     int a[sizeof(__CONCAT(__you_must_first_make_asym_, asym))]; \
	 };                                                              \
         asm("\n.globl " __AROS_STR(asym) "\n")
#endif

#endif /* AROS_SYSTEM_H */
