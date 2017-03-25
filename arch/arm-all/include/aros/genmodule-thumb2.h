/*
    Copyright © 2016-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: genmodule-thumb2.h include file for arm-le systems in thumb2 mode
    Lang: english
*/

#ifndef AROS_ARM_GENMODULE_H
#define AROS_ARM_GENMODULE_H

#include <exec/execbase.h>

/* Macros for generating library stub functions and aliases for stack libcalls. */

/******************* Linklib Side Thunks ******************/

/* Macro: AROS_GM_LIBFUNCSTUB(functionname, libbasename, lvo)
   This macro will generate code for a stub function for
   the function 'functionname' of lirary with libbase
   'libbasename' and 'lvo' number of the function in the
   vector table. lvo has to be a constant value (not a variable)
*/
#define __AROS_GM_LIBFUNCSTUB(fname, libbasename, lvo)                  \
    void __attribute__((noreturn)) __ ## fname ## _ ## libbasename ## _wrapper(void)              \
    {                                                                   \
        asm volatile(                                                   \
            ".thumb \n"                                                 \
            ".weak " #fname "\n"                                        \
            ".type " #fname ", %%function\n"                            \
            #fname " :\n"                                               \
            /* r12 = libbase */                                         \
            "\tldr r12, 1f\n"                                           \
            "\tldr r12, [r12]\n"                                        \
            /* Compute function address and jump */                     \
            "\tsub.w r12, r12, #%c0\n"                                  \
            "\tbx r12\n"                                                \
            ".align 2\n"                                                \
            "1: .word " #libbasename "\n"                               \
            : : "i" ((lvo*LIB_VECTSIZE))                               \
        );                                                              \
        __builtin_unreachable();                                        \
    }
#define AROS_GM_LIBFUNCSTUB(fname, libbasename, lvo) \
    __AROS_GM_LIBFUNCSTUB(fname, libbasename, lvo)

/* Macro: AROS_GM_RELLIBFUNCSTUB(functionname, libbasename, lvo)
   Same as AROS_GM_LIBFUNCSTUB but finds libbase at an offset in
   the current libbase
*/
#define __AROS_GM_RELLIBFUNCSTUB(fname, libbasename, lvo)               \
    void __attribute__((noreturn)) __ ## fname ## _ ## libbasename ## _relwrapper(IPTR args)      \
    {                                                                   \
        asm volatile(                                                   \
            ".thumb \n"                                                 \
            ".weak " #fname "\n"                                        \
            ".type " #fname ", %%function\n"                            \
            #fname " :\n"                                               \
            /* return address is in lr register */                      \
            /* Up to four parameters are in r0 - r3 , the rest are on stack */ \
            "\tpush {r0, r1, r2, r3, lr}\n"                             \
            /* r0 = __aros_getoffsettable() */                          \
            "\tbl  __aros_getoffsettable\n"                             \
            /* r12 = libbase */                                         \
            "\tldr r1, 1f\n"                                            \
            "\tldr r1, [r1]\n"                                          \
            "\tldr r12, [r0, r1]\n"                                     \
            /* Restore original arguments */                            \
            "\tpop {r0, r1, r2, r3, lr}\n"                              \
            /* Compute function address and jump */                     \
            "\tsub.w r12, r12, #%c0\n"                                  \
            "\tbx r12\n"                                                \
	    "1:	.word __aros_rellib_offset_" #libbasename "\n"              \
            "2: .word __aros_getoffsettable\n"                          \
            : : "i" ((lvo*LIB_VECTSIZE))                                \
        );                                                              \
        __builtin_unreachable();                                        \
    }
#define AROS_GM_RELLIBFUNCSTUB(fname, libbasename, lvo) \
    __AROS_GM_RELLIBFUNCSTUB(fname, libbasename, lvo)

/* Macro: AROS_GM_LIBFUNCALIAS(functionname, alias)
   This macro will generate an alias 'alias' for function
   'functionname'
*/
#define __AROS_GM_LIBFUNCALIAS(fname, alias) \
    asm(".thumb \n"                 \
        ".weak " #alias "\n"        \
        "\t.set " #alias "," #fname \
    );
#define AROS_GM_LIBFUNCALIAS(fname, alias) \
    __AROS_GM_LIBFUNCALIAS(fname, alias)

/******************* Library Side Thunks ******************/

/* This macro relies upon the fact that the
 * caller to a stack function will have passed in
 * the base in %r12, since the caller will
 * have used the AROS_LIBFUNCSTUB() macro.
 */
#define __GM_STRINGIZE(x) #x
#define __AROS_GM_STACKCALL(fname, libbasename, libfuncname)                    \
    void libfuncname(void);                                                     \
    void __attribute__((noreturn)) __ ## fname ## _stackcall(void)              \
    {                                                                           \
        asm volatile(                                                           \
            ".thumb \n"                                                         \
            "\t" __GM_STRINGIZE(libfuncname) " :\n"                             \
            /* Up to four parameters are in r0 - r3 , the rest are on stack */  \
            "\tpush {r0, r1, r2, r3, lr}\n"                                     \
            "\tmov  r0, r12\n"                                                  \
            "\tbl   __aros_setoffsettable\n"                                    \
            "\tpop  {r0, r1, r2, r3, lr}\n"                                     \
            "\tb   " #fname "\n"                                                \
        );                                                                      \
        __builtin_unreachable();                                                \
    }
    
#define AROS_GM_STACKCALL(fname, libbasename, lvo) \
     __AROS_GM_STACKCALL(fname, libbasename, AROS_SLIB_ENTRY(fname, libbasename, lvo))

/* Macro: AROS_GM_STACKALIAS(functionname, libbasename, lvo)
   This macro will generate an alias 'alias' for function
   'functionname'
*/
#define __AROS_GM_STACKALIAS(fname, alias)          \
    void alias(void);                               \
    asm(".thumb \n"                                 \
        ".weak " __GM_STRINGIZE(alias) "\n"         \
        "\t.set " __GM_STRINGIZE(alias) "," #fname  \
    );
#define AROS_GM_STACKALIAS(fname, libbasename, lvo) \
    __AROS_GM_STACKALIAS(fname, AROS_SLIB_ENTRY(fname, libbasename, lvo))

#endif /* AROS_ARM_GENMODULE_H */
