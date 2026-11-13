/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: genmodule.h include file for arm-le systems
*/

#ifndef AROS_ARM_GENMODULE_H
#define AROS_ARM_GENMODULE_H

#include <exec/execbase.h>

/* Macros for generating library stub functions and aliases for stack libcalls. */

/******************* Linklib Side Thunks ******************/

/* Tail shared by the two linklib stubs below: jump to JumpVec[-lvo] with the
   libbase left in r12, where the library side thunk expects it.

   ARM's ldr immediate offset is only 12 bits, so the direct form reaches just
   the first 1024 vectors. Past that the offset has to come out of a literal,
   and r12 is the sole register a stub may clobber - r0-r3 carry the arguments,
   r4-r11 are callee saved, lr is the return address and r12 itself has to
   arrive holding the base - so the target address is staged through the stack
   instead of a second scratch register. Both paths key off operand 0, which is
   the (negative) vector offset in each of the macros.  */
#define __AROS_GM_STUBJMP                                               \
            ".if (%c0) >= -4095\n"                                      \
            "\tldr pc, [r12, #%c0]\n"                                   \
            ".else\n"                                                   \
            "\tpush {r0}\n"             /* [sp] = r0                 */ \
            "\tldr r0, 3f\n"            /* r0 = -lvo*LIB_VECTSIZE    */ \
            "\tldr r0, [r12, r0]\n"     /* r0 = JumpVec[-lvo].vec    */ \
            "\tstr r0, [sp, #-4]!\n"    /* [sp] = target, [sp+4] = r0 */\
            "\tldr r0, [sp, #4]\n"      /* restore r0                */ \
            "\tldr pc, [sp], #8\n"      /* jump, releasing both slots */ \
            ".endif\n"

/* Literal holding the vector offset, emitted only when the tail above needs
   it. Placed with the stub's other literals, past the end of the code.  */
#define __AROS_GM_STUBLIT                                               \
            ".if (%c0) < -4095\n"                                       \
            ".align 2\n"                                                \
            "3:\t.word %c0\n"                                           \
            ".endif\n"

/* Macro: AROS_GM_LIBFUNCSTUB(functionname, libbasename, lvo)
   This macro will generate code for a stub function for
   the function 'functionname' of lirary with libbase
   'libbasename' and 'lvo' number of the function in the
   vector table. lvo has to be a constant value (not a variable)
*/
#define __AROS_GM_LIBFUNCSTUB(fname, libbasename, lvo)                     \
    void __ ## fname ## _ ## libbasename ## _wrapper(void)              \
    {                                                                   \
        asm volatile(                                                   \
            ".weak " #fname "\n"                                        \
            ".type " #fname ", %%function\n"                            \
            #fname " :\n"                                               \
            /* r12 = libbase */                                         \
            "\tldr r12, 1f\n"                                           \
            "\tldr r12, [r12]\n"                                        \
            /* Compute function address and jump */                     \
            __AROS_GM_STUBJMP                                           \
            ".align 2\n"                                                \
            "1: .word " #libbasename "\n"                               \
            __AROS_GM_STUBLIT                                           \
            : : "i" ((-lvo*LIB_VECTSIZE))                               \
        );                                                              \
    }
#define AROS_GM_LIBFUNCSTUB(fname, libbasename, lvo) \
    __AROS_GM_LIBFUNCSTUB(fname, libbasename, lvo)

/* Macro: AROS_GM_RELLIBFUNCSTUB(functionname, libbasename, lvo)
   Same as AROS_GM_LIBFUNCSTUB but finds libbase at an offset in
   the current libbase
*/
#define __AROS_GM_RELLIBFUNCSTUB(fname, libbasename, lvo)                  \
    void __ ## fname ## _ ## libbasename ## _relwrapper(IPTR args)      \
    {                                                                   \
        asm volatile(                                                   \
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
            __AROS_GM_STUBJMP                                           \
            ".align 2\n"                                                \
	    "1:	.word __aros_rellib_offset_" #libbasename "\n"                        \
            "2: .word __aros_getoffsettable\n"                          \
            __AROS_GM_STUBLIT                                           \
            : : "i" ((-lvo*LIB_VECTSIZE))                               \
        );                                                              \
    }
#define AROS_GM_RELLIBFUNCSTUB(fname, libbasename, lvo) \
    __AROS_GM_RELLIBFUNCSTUB(fname, libbasename, lvo)

/* Macro: AROS_GM_LIBFUNCALIAS(functionname, alias)
   This macro will generate an alias 'alias' for function
   'functionname'
*/
#define __AROS_GM_LIBFUNCALIAS(fname, alias) \
    asm(".weak " #alias "\n" \
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
#define __AROS_GM_STACKCALL(fname, libbasename, libfuncname)               \
    void libfuncname(void);                                             \
    void __ ## fname ## _stackcall(void)                                \
    {                                                                   \
        asm volatile(                                                   \
            "\t" __GM_STRINGIZE(libfuncname) " :\n"                        \
            /* Up to four parameters are in r0 - r3 , the rest are on stack */ \
            "\tpush {r0, r1, r2, r3, lr}\n"                             \
            "\tmov  r0, r12\n"                                          \
            "\tbl   __aros_setoffsettable\n"                            \
            "\tpop  {r0, r1, r2, r3, lr}\n"                             \
            "\tb   " #fname "\n"                                        \
        );                                                              \
    }
    
#define AROS_GM_STACKCALL(fname, libbasename, lvo) \
     __AROS_GM_STACKCALL(fname, libbasename, AROS_SLIB_ENTRY(fname, libbasename, lvo))

/* Macro: AROS_GM_STACKALIAS(functionname, libbasename, lvo)
   This macro will generate an alias 'alias' for function
   'functionname'
*/
#define __AROS_GM_STACKALIAS(fname, alias) \
    void alias(void); \
    asm(".weak " __GM_STRINGIZE(alias) "\n" \
        "\t.set " __GM_STRINGIZE(alias) "," #fname \
    );
#define AROS_GM_STACKALIAS(fname, libbasename, lvo) \
    __AROS_GM_STACKALIAS(fname, AROS_SLIB_ENTRY(fname, libbasename, lvo))

#endif /* AROS_ARM_GENMODULE_H */
