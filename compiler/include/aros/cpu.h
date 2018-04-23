#ifndef AROS_CPU_H
#define AROS_CPU_H
/*
    Copyright © 1995-2016, The AROS Development Team. All rights reserved.
    $Id$

    CPU independent version of the <aros/cpu.h> header. This is the one
    that normal programs can include.
*/

#define AROS_CPU_IA32               1
#define AROS_CPU_M68K               2
#define AROS_CPU_PPC32              3
#define AROS_CPU_PPC64              4
#define AROS_CPU_AXP                5
#define AROS_CPU_SPARC32            6
#define AROS_CPU_SPARC64            7
#define AROS_CPU_IA64               8
#define AROS_CPU_X8664              9
#define AROS_CPU_ARM				10

/* Fix up __powerpc__ definition if missing */
#ifdef __ppc__
#ifndef __powerpc__
#define __powerpc__
#endif
#endif

/*
    Firstly, include the sub-include file for a particular CPU.
*/
#if defined __i386__
#   include <aros/i386/cpu.h>
#elif defined __x86_64__
#   include <aros/x86_64/cpu.h>
#elif defined __mc68000__
#   include <aros/m68k/cpu.h>
#elif defined __MORPHOS__
#   include <aros/morphos/cpu.h>
#elif defined __powerpc__
#   include <aros/ppc/cpu.h>
#elif defined __aarch64__ 
#   include <aros/aarch64/cpu.h>
#elif defined __arm__
#   if defined __thumb2__
#	    include <aros/arm/cpu-thumb2.h>
#   else
#	    include <aros/arm/cpu.h>
#   endif
#else
#   error unsupported CPU type
#endif

#if !AROS_STACK_GROWS_DOWNWARDS
#error Several places in AROS code assume a stack that grows downwards
#endif

/*
    Under 64-bit MS Windows long is still 32 bits
*/

#ifdef _WIN64
#define AROS_INTPTR_TYPE long long
#define AROS_INTPTR_STACKTYPE long long
#define AROS_LARGEST_TYPE long long
#endif

/*
    Now, for any optional define that hasn't been provided, we must provide
    an implementation of it here. This is somewhat tedious...
*/

#ifndef AROS_INTPTR_TYPE
#define AROS_INTPTR_TYPE    long
#endif

#ifndef AROS_32BIT_TYPE
#define AROS_32BIT_TYPE     int
#endif

#ifndef AROS_16BIT_TYPE
#define AROS_16BIT_TYPE     short
#endif

#ifndef AROS_8BIT_TYPE
#define AROS_8BIT_TYPE      char
#endif

#ifndef AROS_64BIT_TYPE
#define AROS_64BIT_TYPE     long long
#endif

#ifndef AROS_INTPTR_STACKTYPE
#define AROS_INTPTR_STACKTYPE       long
#endif

#ifndef AROS_64BIT_STACKTYPE
#define AROS_64BIT_STACKTYPE        long long
#endif

#ifndef AROS_32BIT_STACKTYPE
#define AROS_32BIT_STACKTYPE        long
#endif

#ifndef AROS_16BIT_STACKTYPE
#define AROS_16BIT_STACKTYPE        long
#endif

#ifndef AROS_8BIT_STACKTYPE
#define AROS_8BIT_STACKTYPE         long
#endif

#ifndef AROS_FLOAT_STACKTYPE
#define AROS_FLOAT_STACKTYPE        float
#endif

#ifndef AROS_DOUBLE_STACKTYPE
#define AROS_DOUBLE_STACKTYPE       double
#endif

#ifndef AROS_LARGEST_TYPE
#define AROS_LARGEST_TYPE           long
#endif

#ifndef AROS_ATOMIC_TYPE
#define AROS_ATOMIC_TYPE            int
#endif

/*
 * AROS_xBIT_LEAST: A type that holds at least a certain bit width.
 */
#ifndef AROS_8BIT_LEASTTYPE
#   define AROS_8BIT_LEASTTYPE              AROS_8BIT_TYPE
#endif
#ifndef AROS_16BIT_LEASTTYPE
#   define AROS_16BIT_LEASTTYPE             AROS_16BIT_TYPE
#endif
#ifndef AROS_32BIT_LEASTTYPE
#   define AROS_32BIT_LEASTTYPE             AROS_32BIT_TYPE
#endif
#ifndef AROS_64BIT_LEASTTYPE
#   ifdef AROS_64BIT_TYPE
#       define AROS_64BIT_LEASTTYPE         AROS_64BIT_TYPE
#   endif
#endif

#ifndef AROS_8BIT_LEASTMIN
#   define AROS_8BIT_LEASTMIN              AROS_8BIT_MIN
#endif
#ifndef AROS_16BIT_LEASTMIN
#   define AROS_16BIT_LEASTMIN             AROS_16BIT_MIN
#endif
#ifndef AROS_32BIT_LEASTMIN
#   define AROS_32BIT_LEASTMIN             AROS_32BIT_MIN
#endif
#ifndef AROS_64BIT_LEASTMIN
#   ifdef AROS_64BIT_MIN
#       define AROS_64BIT_LEASTMIN         AROS_64BIT_MIN
#   endif
#endif

#ifndef AROS_8BIT_LEASTMAX
#   define AROS_8BIT_LEASTMAX              AROS_8BIT_MAX
#endif
#ifndef AROS_16BIT_LEASTMAX
#   define AROS_16BIT_LEASTMAX             AROS_16BIT_MAX
#endif
#ifndef AROS_32BIT_LEASTMAX
#   define AROS_32BIT_LEASTMAX             AROS_32BIT_MAX
#endif
#ifndef AROS_64BIT_LEASTMAX
#   ifdef AROS_64BIT_MAX
#       define AROS_64BIT_LEASTMAX         AROS_64BIT_MAX
#   endif
#endif


/*
 * AROS_xBIT_FAST: A type that is fast for operating quickly
 */
#ifndef AROS_8BIT_FASTTYPE
#   define AROS_8BIT_FASTTYPE               AROS_8BIT_TYPE
#endif
#ifndef AROS_16BIT_FASTTYPE
#   define AROS_16BIT_FASTTYPE              AROS_16BIT_TYPE
#endif
#ifndef AROS_32BIT_FASTTYPE
#   define AROS_32BIT_FASTTYPE              AROS_32BIT_TYPE
#endif
#ifndef AROS_64BIT_FASTTYPE
#   ifdef AROS_64BIT_TYPE
#       define AROS_64BIT_FASTTYPE          AROS_64BIT_TYPE
#   endif
#endif

#ifndef AROS_8BIT_FASTMIN
#   define AROS_8BIT_FASTMIN               AROS_8BIT_MIN
#endif
#ifndef AROS_16BIT_FASTMIN
#   define AROS_16BIT_FASTMIN              AROS_16BIT_MIN
#endif
#ifndef AROS_32BIT_FASTMIN
#   define AROS_32BIT_FASTMIN              AROS_32BIT_MIN
#endif
#ifndef AROS_64BIT_FASTMIN
#   ifdef AROS_64BIT_MIN
#       define AROS_64BIT_FASTMIN          AROS_64BIT_MIN
#   endif
#endif

#ifndef AROS_8BIT_FASTMAX
#   define AROS_8BIT_FASTMAX               AROS_8BIT_MAX
#endif
#ifndef AROS_16BIT_FASTMAX
#   define AROS_16BIT_FASTMAX              AROS_16BIT_MAX
#endif
#ifndef AROS_32BIT_FASTMAX
#   define AROS_32BIT_FASTMAX              AROS_32BIT_MAX
#endif
#ifndef AROS_64BIT_FASTMAX
#   ifdef AROS_64BIT_MAX
#       define AROS_64BIT_FASTMAX          AROS_64BIT_MAX
#   endif
#endif

/*
    The SP_OFFSET should be defined *ONLY* when not defined before.
    Otherwise it would redefine defaults from cpu-arch.h or machine.h file
*/
#ifndef SP_OFFSET
#define SP_OFFSET       0
#endif /* SP_OFFSET */

#ifndef AROS_COMPAT_SETD0
#define AROS_COMPAT_SETD0(x) (void)x
#endif /* AROS_COMPAT_SETD0 */

/* These macros will produce a value that can be stored in a AROS_64BIT_TYPE */
#ifndef AROS_MAKE_INT64
#define AROS_MAKE_INT64(i)  i ## LL
#endif
#ifndef AROS_MAKE_UINT64
#define AROS_MAKE_UINT64(i) i ## ULL
#endif

#ifndef __WORDSIZE
#define __WORDSIZE 32
#endif

#ifndef STACKED
#define STACKED __attribute__((aligned(4)))
#endif

#ifndef AROS_HOST_BARRIER
#define AROS_HOST_BARRIER
#endif

#endif /* AROS_CPU_H */
