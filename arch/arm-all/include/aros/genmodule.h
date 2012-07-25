#ifndef AROS_ARM_GENMODULE_H
#define AROS_ARM_GENMODULE_H

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    NOTE: This file must compile *without* any other header !

    Desc: genmodule.h include file for arm-le systems
    Lang: english
*/

#include <exec/execbase.h>

/* Macros for generating library stub functions and aliases for stack libcalls. */

/* Macro: AROS_LIBFUNCSTUB(functionname, libbasename, lvo)
   This macro will generate code for a stub function for
   the function 'functionname' of lirary with libbase
   'libbasename' and 'lvo' number of the function in the
   vector table. lvo has to be a constant value (not a variable)

   Some asm trickery performed:
   - Push register arguments on stack
   - Use aros_push2_relbase to store old return address (LR register)
     and library base on alternative stack
   - Pop register arguments back
   - Call lvo vector
   - push return value on stack
   - call aros_pop2_relbase,
     return value will be old return address
   - pull return value from stack
   - jmp to old return address
*/
#define __AROS_LIBFUNCSTUB(fname, libbasename, lvo)                     \
    void __ ## fname ## _ ## libbasename ## _wrapper(void)              \
    {                                                                   \
        asm volatile(                                                   \
            ".weak " #fname "\n"                                        \
	    #fname " :\n"						\
	    /* r12 = libbase */                                         \
	    "\tldr r12, 1f\n"                                           \
            "\tldr r12, [r12]\n"                                        \
            /* Compute function address and jump */                     \
            "\tldr pc, [r12, #%c0]\n"                                   \
            "1:	.word " #libbasename "\n"                               \
            : : "i" ((-lvo*LIB_VECTSIZE))                               \
        );                                                              \
    }
#define AROS_LIBFUNCSTUB(fname, libbasename, lvo) \
    __AROS_LIBFUNCSTUB(fname, libbasename, lvo)

/* Macro: AROS_RELLIBFUNCSTUB(functionname, libbasename, lvo)
   Same as AROS_LIBFUNCSTUB but finds libbase at an offset in
   the current libbase
*/
#define __AROS_RELLIBFUNCSTUB(fname, libbasename, lvo)                  \
    void __ ## fname ## _ ## libbasename ## _relwrapper(IPTR args)      \
    {                                                                   \
        asm volatile(                                                   \
	    ".weak " #fname "\n"					\
            #fname " :\n"                                               \
            /* return address is in lr register */                      \
            /* Up to four parameters are in r0 - r3 , the rest are on stack */ \
            "\tpush {r0, r1, r2, r3, lr}\n"                             \
            /* r0 = __GM_GetBase() */                             \
            "\tldr r12, 2f\n"                                           \
            "\tblx r12\n"                                               \
            /* r12 = libbase */                                         \
            "\tldr r1, 1f\n"                                            \
            "\tldr r1, [r1]\n"                                          \
            "\tldr r12, [r0, r1]\n"                                     \
            /* Restore original arguments */                            \
            "\tpop {r0, r1, r2, r3, lr}\n"                              \
            /* Compute function address and jump */                     \
            "\tldr pc, [r12, #%c0]\n"                                   \
	    "1:	.word " #libbasename "_offset\n"                        \
            "2: .word __GM_GetBase\n"                             \
            : : "i" ((-lvo*LIB_VECTSIZE))                               \
        );                                                              \
    }
#define AROS_RELLIBFUNCSTUB(fname, libbasename, lvo) \
    __AROS_RELLIBFUNCSTUB(fname, libbasename, lvo)

/* Macro: AROS_FUNCALIAS(functionname, alias)
   This macro will generate an alias 'alias' for function
   'functionname'
*/
#define __AROS_FUNCALIAS(fname, alias) \
    asm(".weak " #alias "\n" \
	"\t.set " #alias "," #fname \
    );
#define AROS_FUNCALIAS(fname, alias) \
    __AROS_FUNCALIAS(fname, alias)

#endif /* AROS_ARM_GENMODULE_H */
