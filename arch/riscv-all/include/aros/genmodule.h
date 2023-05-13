/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: genmodule.h include file for risc-v systems
*/

#ifndef AROS_RISCV_GENMODULE_H
#define AROS_RISCV_GENMODULE_H

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
    void __ ## fname ## _ ## libbasename ## _wrapper(void)              \
    {                                                                   \
        asm volatile(                                                   \
            ".weak " #fname "\n"                                        \
            ".type " #fname ", %%function\n"                            \
            #fname " :\n"                                               \
            /* a7 = libbase */                                          \
            "\tla a7, 1f\n"                                             \
            "\tlw a7, 0(a7)\n"                                          \
            /* Compute function address and jump */                     \
            "\tjal a7, %0\n"                                            \
            ".align 2\n"                                                \
            "1: .word " #libbasename "\n"                               \
            : : "i" ((-lvo*LIB_VECTSIZE))                               \
        );                                                              \
    }
#define AROS_GM_LIBFUNCSTUB(fname, libbasename, lvo) \
    __AROS_GM_LIBFUNCSTUB(fname, libbasename, lvo)

/* Macro: AROS_GM_RELLIBFUNCSTUB(functionname, libbasename, lvo)
   Same as AROS_GM_LIBFUNCSTUB but finds libbase at an offset in
   the current libbase
*/
#define __AROS_GM_RELLIBFUNCSTUB(fname, libbasename, lvo)               \
    void __ ## fname ## _ ## libbasename ## _relwrapper(IPTR args)      \
    {                                                                   \
        asm volatile(                                                   \
            ".weak " #fname "\n"                                        \
            ".type " #fname ", %%function\n"                            \
            #fname " :\n"                                               \
            /* return address is in ra register */                      \
            /* Up to four parameters are in */                          \
            /* a0 - a3 , the rest are on stack */                       \
            "\taddi sp,sp,-20\n"                                        \
            "\tsw a0, 16(sp)\n"                                         \
            "\tsw a1, 12(sp)\n"                                         \
            "\tsw a2, 8(sp)\n"                                          \
            "\tsw a3, 4(sp)\n"                                          \
            "\tsw ra, 0(sp)\n"                                          \
            /* a0 = __aros_getoffsettable() */                          \
            "\tcall  __aros_getoffsettable\n"                           \
            /* a7 = libbase */                                          \
            "\tla a1, 1f\n"                                             \
            "\tlw a1, 0(a1)\n"                                          \
            "\tadd a7, a0, a1\n"                                        \
            /* Restore original arguments */                            \
            "\tlw ra, 0(sp)\n"                                          \
            "\tlw a3, 4(sp)\n"                                          \
            "\tlw a2, 8(sp)\n"                                          \
            "\tlw a1, 12(sp)\n"                                         \
            "\tlw a0, 16(sp)\n"                                         \
            "\taddi sp,sp,20\n"                                         \
            /* Compute function address and jump */                     \
            "\tjal a7, %0\n"                                            \
	    "1:	.word __aros_rellib_offset_" #libbasename "\n"              \
            "2: .word __aros_getoffsettable\n"                          \
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
 * the base in %a7, since the caller will
 * have used the AROS_LIBFUNCSTUB() macro.
 */
#define __GM_STRINGIZE(x) #x
#define __AROS_GM_STACKCALL(fname, libbasename, libfuncname)            \
    void libfuncname(void);                                             \
    void __ ## fname ## _stackcall(void)                                \
    {                                                                   \
        asm volatile(                                                   \
            "\t" __GM_STRINGIZE(libfuncname) " :\n"                     \
            "\taddi sp,sp,-20\n"                                        \
            "\tsw a0, 16(sp)\n"                                         \
            "\tsw a1, 12(sp)\n"                                         \
            "\tsw a2, 8(sp)\n"                                          \
            "\tsw a3, 4(sp)\n"                                          \
            "\tsw ra, 0(sp)\n"                                          \
            "\tmv  a0, a7\n"                                            \
            "\tcall   __aros_setoffsettable\n"                          \
            "\tlw ra, 0(sp)\n"                                          \
            "\tlw a3, 4(sp)\n"                                          \
            "\tlw a2, 8(sp)\n"                                          \
            "\tlw a1, 12(sp)\n"                                         \
            "\tlw a0, 16(sp)\n"                                         \
            "\taddi sp,sp,20\n"                                         \
            "\tjal   " #fname "\n"                                      \
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

#endif /* AROS_RISCV_GENMODULE_H */
