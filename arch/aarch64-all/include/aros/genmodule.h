/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: genmodule.h include file for aarch64 (AArch64/ARM64-le) systems.

    AArch64 translation of the arm-all version. Library-call stubs jump through
    the library's downward-growing vector table: the function for LVO n lives at
    (libbase - n*LIB_VECTSIZE), and each JumpVec slot holds a 64-bit function
    pointer. x16/x17 (IP0/IP1) are the AAPCS64 intra-procedure-call scratch
    registers, free to clobber inside a tail-call thunk.
*/

#ifndef AROS_AARCH64_GENMODULE_H
#define AROS_AARCH64_GENMODULE_H

#include <exec/execbase.h>

/* Macros for generating library stub functions and aliases for stack libcalls. */

/******************* Linklib Side Thunks ******************/

/* Macro: AROS_GM_LIBFUNCSTUB(functionname, libbasename, lvo)
   Generates a stub for 'functionname' of the library whose base pointer is the
   global 'libbasename'. It loads the base, indexes the vector table at -lvo and
   tail-jumps to the function. lvo must be a compile-time constant.
*/
#define __AROS_GM_LIBFUNCSTUB(fname, libbasename, lvo)                     \
    void __ ## fname ## _ ## libbasename ## _wrapper(void)                 \
    {                                                                      \
        asm volatile(                                                      \
            ".weak " #fname "\n"                                           \
            ".type " #fname ", %%function\n"                               \
            #fname " :\n"                                                  \
            "\tldr  x16, 1f\n"          /* x16 = &libbasename           */ \
            "\tldr  x16, [x16]\n"       /* x16 = libbase                */ \
            "\tmov  x17, #%c0\n"        /* x17 = lvo*LIB_VECTSIZE        */ \
            "\tsub  x17, x16, x17\n"    /* x17 = &JumpVec[-lvo]          */ \
            "\tldr  x17, [x17]\n"       /* x17 = function pointer        */ \
            "\tbr   x17\n"                                                 \
            "\t.align 3\n"                                                 \
            "1:\t.quad " #libbasename "\n"                                 \
            : : "i" ((lvo)*LIB_VECTSIZE)                                   \
        );                                                                 \
    }
#define AROS_GM_LIBFUNCSTUB(fname, libbasename, lvo) \
    __AROS_GM_LIBFUNCSTUB(fname, libbasename, lvo)

/* Macro: AROS_GM_RELLIBFUNCSTUB(functionname, libbasename, lvo)
   Same as AROS_GM_LIBFUNCSTUB but resolves the libbase through the per-task
   offset table (__aros_getoffsettable + __aros_rellib_offset_<libbasename>).
*/
#define __AROS_GM_RELLIBFUNCSTUB(fname, libbasename, lvo)                  \
    void __ ## fname ## _ ## libbasename ## _relwrapper(IPTR args)         \
    {                                                                      \
        asm volatile(                                                      \
            ".weak " #fname "\n"                                           \
            ".type " #fname ", %%function\n"                               \
            #fname " :\n"                                                  \
            /* Preserve every argument-carrying register across the       \
             * helper call: x0-x7 (integer args), q0-q7 (FP/SIMD args),   \
             * x8 (indirect result location) and lr. AAPCS64 lets the     \
             * callee clobber all of them. */                             \
            "\tstp  q0, q1, [sp, #-208]!\n"                               \
            "\tstp  q2, q3, [sp, #32]\n"                                  \
            "\tstp  q4, q5, [sp, #64]\n"                                  \
            "\tstp  q6, q7, [sp, #96]\n"                                  \
            "\tstp  x0, x1, [sp, #128]\n"                                 \
            "\tstp  x2, x3, [sp, #144]\n"                                 \
            "\tstp  x4, x5, [sp, #160]\n"                                 \
            "\tstp  x6, x7, [sp, #176]\n"                                 \
            "\tstp  x8, x30, [sp, #192]\n"                                \
            "\tbl   __aros_getoffsettable\n"  /* x0 = offset table       */ \
            "\tldr  x16, 1f\n"                /* x16 = &rellib_offset    */ \
            "\tldr  x16, [x16]\n"             /* x16 = offset value      */ \
            "\tldr  x16, [x0, x16]\n"         /* x16 = libbase           */ \
            "\tldp  x8, x30, [sp, #192]\n"                                \
            "\tldp  x6, x7, [sp, #176]\n"                                 \
            "\tldp  x4, x5, [sp, #160]\n"                                 \
            "\tldp  x2, x3, [sp, #144]\n"                                 \
            "\tldp  x0, x1, [sp, #128]\n"                                 \
            "\tldp  q6, q7, [sp, #96]\n"                                  \
            "\tldp  q4, q5, [sp, #64]\n"                                  \
            "\tldp  q2, q3, [sp, #32]\n"                                  \
            "\tldp  q0, q1, [sp], #208\n"                                 \
            "\tmov  x17, #%c0\n"                                           \
            "\tsub  x17, x16, x17\n"          /* x17 = &JumpVec[-lvo]    */ \
            "\tldr  x17, [x17]\n"             /* x17 = function pointer  */ \
            "\tbr   x17\n"                                                 \
            "\t.align 3\n"                                                 \
            "1:\t.quad __aros_rellib_offset_" #libbasename "\n"            \
            : : "i" ((lvo)*LIB_VECTSIZE)                                   \
        );                                                                 \
    }
#define AROS_GM_RELLIBFUNCSTUB(fname, libbasename, lvo) \
    __AROS_GM_RELLIBFUNCSTUB(fname, libbasename, lvo)

/* Macro: AROS_GM_LIBFUNCALIAS(functionname, alias)
   Generates a weak alias 'alias' for 'functionname' (CPU-independent).
*/
#define __AROS_GM_LIBFUNCALIAS(fname, alias) \
    asm(".weak " #alias "\n" \
        "\t.set " #alias "," #fname \
    );
#define AROS_GM_LIBFUNCALIAS(fname, alias) \
    __AROS_GM_LIBFUNCALIAS(fname, alias)

/******************* Library Side Thunks ******************/

/* Relies upon the caller (a LIBFUNCSTUB above) having left the libbase in x16.
 * Records it via __aros_setoffsettable then tail-branches to the real function.
 */
#define __GM_STRINGIZE(x) #x
#define __AROS_GM_STACKCALL(fname, libbasename, libfuncname)               \
    void libfuncname(void);                                                \
    void __ ## fname ## _stackcall(void)                                   \
    {                                                                      \
        asm volatile(                                                      \
            "\t" __GM_STRINGIZE(libfuncname) " :\n"                        \
            /* Preserve x0-x7, q0-q7, x8 (indirect result) and lr --      \
             * AAPCS64 lets the callee clobber all of them. */            \
            "\tstp  q0, q1, [sp, #-208]!\n"                               \
            "\tstp  q2, q3, [sp, #32]\n"                                  \
            "\tstp  q4, q5, [sp, #64]\n"                                  \
            "\tstp  q6, q7, [sp, #96]\n"                                  \
            "\tstp  x0, x1, [sp, #128]\n"                                 \
            "\tstp  x2, x3, [sp, #144]\n"                                 \
            "\tstp  x4, x5, [sp, #160]\n"                                 \
            "\tstp  x6, x7, [sp, #176]\n"                                 \
            "\tstp  x8, x30, [sp, #192]\n"                                \
            "\tmov  x0, x16\n"                /* arg0 = libbase          */ \
            "\tbl   __aros_setoffsettable\n"                               \
            "\tldp  x8, x30, [sp, #192]\n"                                \
            "\tldp  x6, x7, [sp, #176]\n"                                 \
            "\tldp  x4, x5, [sp, #160]\n"                                 \
            "\tldp  x2, x3, [sp, #144]\n"                                 \
            "\tldp  x0, x1, [sp, #128]\n"                                 \
            "\tldp  q6, q7, [sp, #96]\n"                                  \
            "\tldp  q4, q5, [sp, #64]\n"                                  \
            "\tldp  q2, q3, [sp, #32]\n"                                  \
            "\tldp  q0, q1, [sp], #208\n"                                 \
            "\tb   " #fname "\n"                                           \
        );                                                                 \
    }
#define AROS_GM_STACKCALL(fname, libbasename, lvo) \
     __AROS_GM_STACKCALL(fname, libbasename, AROS_SLIB_ENTRY(fname, libbasename, lvo))

/* Macro: AROS_GM_STACKALIAS(functionname, libbasename, lvo)
   Generates a weak alias for the library-side entry of 'functionname'.
*/
#define __AROS_GM_STACKALIAS(fname, alias) \
    void alias(void); \
    asm(".weak " __GM_STRINGIZE(alias) "\n" \
        "\t.set " __GM_STRINGIZE(alias) "," #fname \
    );
#define AROS_GM_STACKALIAS(fname, libbasename, lvo) \
    __AROS_GM_STACKALIAS(fname, AROS_SLIB_ENTRY(fname, libbasename, lvo))

#endif /* AROS_AARCH64_GENMODULE_H */
