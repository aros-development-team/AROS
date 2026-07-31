/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: genmodule.h include file for 64bit risc-v (RV64, LP64D) systems.

    Library-call stubs jump through the library's downward-growing vector
    table: the function for LVO n lives at (libbase - n*LIB_VECTSIZE), and
    each JumpVec slot holds a 64-bit function pointer. The stubs hand the
    libbase to the library-side thunk in t6 - a temporary register that is
    never used to pass arguments, so all eight integer (a0-a7) and eight FP
    (fa0-fa7) argument registers stay untouched.
*/

#ifndef AROS_RISCV64_GENMODULE_H
#define AROS_RISCV64_GENMODULE_H

#include <exec/execbase.h>

/* Macros for generating library stub functions and aliases for stack libcalls. */

/******************* Linklib Side Thunks ******************/

/* Macro: AROS_GM_LIBFUNCSTUB(functionname, libbasename, lvo)
   Generates a stub for 'functionname' of the library whose base pointer is
   the global 'libbasename'. It loads the base, indexes the vector table at
   -lvo and tail-jumps to the function. lvo must be a compile-time constant.
*/
#define __AROS_GM_LIBFUNCSTUB(fname, libbasename, lvo)                     \
    void __ ## fname ## _ ## libbasename ## _wrapper(void)                 \
    {                                                                      \
        asm volatile(                                                      \
            ".weak " #fname "\n"                                           \
            ".type " #fname ", %%function\n"                               \
            #fname " :\n"                                                  \
            "\tla   t6, " #libbasename "\n" /* t6 = &libbasename        */ \
            "\tld   t6, 0(t6)\n"            /* t6 = libbase             */ \
            "\tli   t0, %0\n"               /* t0 = lvo*LIB_VECTSIZE    */ \
            "\tsub  t0, t6, t0\n"           /* t0 = &JumpVec[-lvo]      */ \
            "\tld   t0, 0(t0)\n"            /* t0 = function pointer    */ \
            "\tjr   t0\n"                                                  \
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
            /* Preserve every argument-carrying register across the        \
             * helper call: a0-a7 (integer args), fa0-fa7 (FP args) and    \
             * ra. The LP64D ABI lets the callee clobber all of them. */   \
            "\taddi sp, sp, -144\n"                                        \
            "\tfsd  fa0, 0(sp)\n"                                          \
            "\tfsd  fa1, 8(sp)\n"                                          \
            "\tfsd  fa2, 16(sp)\n"                                         \
            "\tfsd  fa3, 24(sp)\n"                                         \
            "\tfsd  fa4, 32(sp)\n"                                         \
            "\tfsd  fa5, 40(sp)\n"                                         \
            "\tfsd  fa6, 48(sp)\n"                                         \
            "\tfsd  fa7, 56(sp)\n"                                         \
            "\tsd   a0, 64(sp)\n"                                          \
            "\tsd   a1, 72(sp)\n"                                          \
            "\tsd   a2, 80(sp)\n"                                          \
            "\tsd   a3, 88(sp)\n"                                          \
            "\tsd   a4, 96(sp)\n"                                          \
            "\tsd   a5, 104(sp)\n"                                         \
            "\tsd   a6, 112(sp)\n"                                         \
            "\tsd   a7, 120(sp)\n"                                         \
            "\tsd   ra, 128(sp)\n"                                         \
            "\tcall __aros_getoffsettable\n" /* a0 = offset table       */ \
            "\tla   t0, 1f\n"                                              \
            "\tld   t0, 0(t0)\n"             /* t0 = rellib offset      */ \
            "\tadd  t0, a0, t0\n"                                          \
            "\tld   t6, 0(t0)\n"             /* t6 = libbase            */ \
            "\tld   ra, 128(sp)\n"                                         \
            "\tld   a7, 120(sp)\n"                                         \
            "\tld   a6, 112(sp)\n"                                         \
            "\tld   a5, 104(sp)\n"                                         \
            "\tld   a4, 96(sp)\n"                                          \
            "\tld   a3, 88(sp)\n"                                          \
            "\tld   a2, 80(sp)\n"                                          \
            "\tld   a1, 72(sp)\n"                                          \
            "\tld   a0, 64(sp)\n"                                          \
            "\tfld  fa7, 56(sp)\n"                                         \
            "\tfld  fa6, 48(sp)\n"                                         \
            "\tfld  fa5, 40(sp)\n"                                         \
            "\tfld  fa4, 32(sp)\n"                                         \
            "\tfld  fa3, 24(sp)\n"                                         \
            "\tfld  fa2, 16(sp)\n"                                         \
            "\tfld  fa1, 8(sp)\n"                                          \
            "\tfld  fa0, 0(sp)\n"                                          \
            "\taddi sp, sp, 144\n"                                         \
            "\tli   t0, %0\n"                /* t0 = lvo*LIB_VECTSIZE   */ \
            "\tsub  t0, t6, t0\n"            /* t0 = &JumpVec[-lvo]     */ \
            "\tld   t0, 0(t0)\n"             /* t0 = function pointer   */ \
            "\tjr   t0\n"                                                  \
            "\t.align 3\n"                                                 \
            "1:\t.dword __aros_rellib_offset_" #libbasename "\n"           \
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

/* Relies upon the caller (a LIBFUNCSTUB above) having left the libbase in
 * t6. Records it via __aros_setoffsettable then tail-jumps to the real
 * function.
 */
#define __GM_STRINGIZE(x) #x
#define __AROS_GM_STACKCALL(fname, libbasename, libfuncname)               \
    void libfuncname(void);                                                \
    void __ ## fname ## _stackcall(void)                                   \
    {                                                                      \
        asm volatile(                                                      \
            "\t" __GM_STRINGIZE(libfuncname) " :\n"                        \
            /* Preserve a0-a7, fa0-fa7 and ra - the LP64D ABI lets the     \
             * callee clobber all of them. */                              \
            "\taddi sp, sp, -144\n"                                        \
            "\tfsd  fa0, 0(sp)\n"                                          \
            "\tfsd  fa1, 8(sp)\n"                                          \
            "\tfsd  fa2, 16(sp)\n"                                         \
            "\tfsd  fa3, 24(sp)\n"                                         \
            "\tfsd  fa4, 32(sp)\n"                                         \
            "\tfsd  fa5, 40(sp)\n"                                         \
            "\tfsd  fa6, 48(sp)\n"                                         \
            "\tfsd  fa7, 56(sp)\n"                                         \
            "\tsd   a0, 64(sp)\n"                                          \
            "\tsd   a1, 72(sp)\n"                                          \
            "\tsd   a2, 80(sp)\n"                                          \
            "\tsd   a3, 88(sp)\n"                                          \
            "\tsd   a4, 96(sp)\n"                                          \
            "\tsd   a5, 104(sp)\n"                                         \
            "\tsd   a6, 112(sp)\n"                                         \
            "\tsd   a7, 120(sp)\n"                                         \
            "\tsd   ra, 128(sp)\n"                                         \
            "\tmv   a0, t6\n"                 /* arg0 = libbase         */ \
            "\tcall __aros_setoffsettable\n"                               \
            "\tld   ra, 128(sp)\n"                                         \
            "\tld   a7, 120(sp)\n"                                         \
            "\tld   a6, 112(sp)\n"                                         \
            "\tld   a5, 104(sp)\n"                                         \
            "\tld   a4, 96(sp)\n"                                          \
            "\tld   a3, 88(sp)\n"                                          \
            "\tld   a2, 80(sp)\n"                                          \
            "\tld   a1, 72(sp)\n"                                          \
            "\tld   a0, 64(sp)\n"                                          \
            "\tfld  fa7, 56(sp)\n"                                         \
            "\tfld  fa6, 48(sp)\n"                                         \
            "\tfld  fa5, 40(sp)\n"                                         \
            "\tfld  fa4, 32(sp)\n"                                         \
            "\tfld  fa3, 24(sp)\n"                                         \
            "\tfld  fa2, 16(sp)\n"                                         \
            "\tfld  fa1, 8(sp)\n"                                          \
            "\tfld  fa0, 0(sp)\n"                                          \
            "\taddi sp, sp, 144\n"                                         \
            "\ttail " #fname "\n"                                          \
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

#endif /* AROS_RISCV64_GENMODULE_H */
