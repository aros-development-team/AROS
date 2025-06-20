#ifndef AROS_SYSTEM_H
#define AROS_SYSTEM_H

/*
    Copyright © 1995-2025, The AROS Development Team. All rights reserved.
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

/*
    __header_inline - Portable inline function marker for header use
    - GCC and Clang: uses static inline + attributes for optimization and dead code removal
    - VBCC / older Amiga compilers: falls back to static inline or just static
*/

#if defined(__GNUC__) || defined(__clang__)
#if !defined(AROS_SAFE_HEADERINLINE)
    #define __header_inline static inline __attribute__((__always_inline__, __unused__))
#else
    #define __header_inline static inline
#endif
#elif defined(__VBCC__)
    /* VBCC supports inline but not always consistently in headers */
    #define __header_inline static inline
#elif defined(__SASC) || defined(__STORM__)
    /* Very old Amiga compilers - no inline support or very limited */
    #define __header_inline static
#else
    /* Generic fallback */
    #define __header_inline static inline
#endif

/*
    Feature detection for compilers like Clang and GCC.
    Allows graceful fallback on older or less feature-rich compilers.
*/

#ifndef __has_builtin
#  define __has_builtin(x) 0
#endif

#ifndef __has_feature
#  define __has_feature(x) 0
#endif

#ifndef __has_extension
#  define __has_extension __has_feature
#endif

#ifndef __has_attribute
#  define __has_attribute(x) 0
#endif

#ifndef __has_c_attribute
#  define __has_c_attribute(x) 0
#endif

/*
    GCC version check macro
    Usage: __GNUC_PREREQ(4, 8) => true if GCC >= 4.8
*/
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#  define __GNUC_PREREQ(maj, min) ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#  define __GNUC_PREREQ(maj, min) 0
#endif

#define __GNUC_PREREQ__ __GNUC_PREREQ

/*
    C++ and extern linkage macros.
    Ensures headers can be used from both C and C++ code without name mangling issues.
*/

#if defined(__cplusplus)
#  define __EXTERN         extern "C"
#  define __BEGIN_DECLS    extern "C" {
#  define __BEGIN_EXTERN   extern "C" {
#  define __END_DECLS      }
#  define __END_EXTERN     }
#else
#  define __EXTERN         extern
#  define __BEGIN_DECLS
#  define __BEGIN_EXTERN
#  define __END_DECLS
#  define __END_EXTERN
#endif

/*
    Handle standard keywords and types across compilers:
    - Define fallback versions of const, inline, volatile, and restrict
    - Only redefine them when not using a modern standard or GCC/Clang
*/

#if defined(__STDC__) || defined(__cplusplus)

#  if !defined(__GNUC__)
#    define __const__     const
#    define __inline__    inline
#    define __volatile__  volatile
#  endif

/*
    C99 defines a new keyword restrict that helps the compiler optimize pointer usage.
    Enable it if the C standard version is sufficient.
*/
#  if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#    define __restrict__ restrict
#  else
#    define __restrict__
#    define restrict
#  endif

#else
/* Older compilers - define keywords to be empty to ensure compatibility */
#  define __const__
#  define const
#  define __inline__
#  define inline
#  define __volatile__
#  define volatile
#  define __restrict__
#  define restrict
#endif

/* Portable fallthrough annotation for switch/case logic */
#ifndef __fallthrough
# if __has_c_attribute(fallthrough)
    # define __fallthrough [[fallthrough]]
# elif __has_attribute(fallthrough) && (__GNUC__ >= 7)
    # define __fallthrough __attribute__((fallthrough))
# else
    /* Older compilers or no support — define as empty */
    # define __fallthrough ((void)0)
# endif
#endif

#if defined(__GNUC__) && !defined(__THROW)
#   if defined __cplusplus && defined(__GNUC__) && __GNUC_PREREQ (2,8)
#       define __THROW       throw ()
#   else
#       define __THROW
#   endif
#endif

/* 3. Macros for making things more efficient */
#ifndef __packed
#if defined(__GNUC__) && __GNUC_PREREQ(2,5)
#   define __packed __attribute__((__packed__))
#else
#   error Define __packed appropriately for your compiler!
#endif
#endif

#ifndef __noreturn
#if defined(__GNUC__) && __GNUC_PREREQ(2,5)
#   define __noreturn  __attribute__((__noreturn__))
#else
#   define __noreturn
#endif
#endif

#ifndef __noeffect
#if defined(__GNUC__) && __GNUC_PREREQ(2,5) && !(__GNUC_PREREQ(2,6) && defined __cplusplus)
#   define __noeffect  __attribute__((__const__))
#else
#   define __noeffect
#endif
#endif

#ifndef __unused
#if defined(__GNUC__) && __GNUC_PREREQ(2,7)
#    define __unused   __attribute__((__unused__))
# else
#    define __unused
# endif
#endif

#ifndef __used
#if defined(__GNUC__) && __GNUC_PREREQ(3,3)
#    define __used   __attribute__((__used__))
# else
#    define __used __unused
# endif
#endif

#ifndef __pure
#if defined(__GNUC__) && __GNUC_PREREQ(2,96)
#    define __pure     __attribute__((__pure__))
#else
#    define __pure
#endif
#endif

/* __const: GCC attribute for side-effect-free functions (not to be confused with C const) */
#ifndef __const
#if defined(__GNUC__) && __GNUC_PREREQ(2,5)
#    define __const     __attribute__((__const__))
#else
#    define __const
#endif
#endif

#ifndef __mayalias
#if defined(__GNUC__) && __GNUC_PREREQ(3,3)
#    define __mayalias  __attribute__((__may_alias__))
#else
#    define __mayalias
#endif
#endif

#define __pure2 __const

/* 4. Macros for debugging and development */
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || (defined(__STDC__) && __STDC_VERSION__ >= 199901L)
#if !defined(AROS_64BIT_TYPE)
#   define AROS_64BIT_TYPE long long
#endif
#   define AROS_HAVE_LONG_LONG
#endif

#if defined(__INTEL_COMPILER)
#   ifdef inline
#       undef inline
#   endif
#   define inline
#endif

#if defined(__GNUC__) && __GNUC__ <= 2
#   define __deprecated
#   define __section(x)
#endif
#if defined(__GNUC__) && __GNUC__ > 2
#   define __deprecated    __attribute__((__deprecated__))
#   define __section(x)    __attribute__((__section__(x)))
#endif

#if defined(__GNUC__) && __GNUC_PREREQ(3,3)
#define __startup __section(".aros.startup") __used
#else
#define __startup __used
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
#   if defined(__ELF__) || defined(__MACH__) || defined(_WIN32) || defined(__CYGWIN__)
#       define AROS_CSYM_FROM_ASM_NAME(n) n
#   else
#       error define AROS_CSYM_FROM_ASM_NAME for your architecture
#   endif
#endif

#include <aros/strmacro.h>

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
