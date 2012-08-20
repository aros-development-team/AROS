#ifndef AROS_X86_64_GENMODULE_H
#define AROS_X86_64_GENMODULE_H

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: genmodule specific definitions for x86_64
    Lang: english
*/

#include <exec/execbase.h>

/* Macros for generating library stub functions and aliases for stack libcalls. */

/* Macro: AROS_LIBFUNCSTUB(functionname, libbasename, lvo)
   This macro will generate code for a stub function for
   the function 'functionname' of library with libbase
   'libbasename' and 'lvo' number of the function in the
   vector table. lvo has to be a constant value (not a variable)

   Internals: a dummy function is used that will generate some
   unused junk code but otherwise we can't pass input arguments
   to the asm statement
*/
#define __AROS_GM_LIBFUNCSTUB(fname, libbasename, lvo) \
    void __ ## fname ## _ ## libbasename ## _wrapper(void) \
    { \
        asm volatile( \
            ".weak " #fname "\n" \
            #fname " :\n" \
            "\tmovq " #libbasename "(%%rip), %%r11\n" \
            "\tjmp  *%c0(%%r11)\n" \
            : : "i" ((-lvo)*LIB_VECTSIZE) \
        ); \
    }
#define AROS_GM_LIBFUNCSTUB(fname, libbasename, lvo) \
    __AROS_GM_LIBFUNCSTUB(fname, libbasename, lvo)

/* Macro: AROS_GM_RELLIBFUNCSTUB(functionname, libbasename, lvo)
   Same as AROS_GM_LIBFUNCSTUB but finds libbase at an offset in
   the current libbase
*/
#define __AROS_GM_RELLIBFUNCSTUB(fname, libbasename, lvo) \
    void __ ## fname ## _ ## libbasename ## _relwrapper(void) \
    { \
        asm volatile( \
            ".weak " #fname "\n" \
            "\t" #fname " :\n"   \
            "\tpushq %%rax\n"     \
            "\tpushq %%rdi\n"     \
            "\tpushq %%rsi\n"     \
            "\tpushq %%rdx\n"     \
            "\tpushq %%rcx\n"     \
            "\tpushq %%r8\n"      \
            "\tpushq %%r9\n"      \
            "\tcall __aros_getbase\n" \
            "\taddq __aros_rellib_offset_" #libbasename "(%%rip), %%rax\n" \
            "\tmovq (%%rax),%%r11\n" \
            "\tpopq %%r9\n"      \
            "\tpopq %%r8\n"      \
            "\tpopq %%rcx\n"     \
            "\tpopq %%rdx\n"     \
            "\tpopq %%rsi\n"     \
            "\tpopq %%rdi\n"     \
            "\tpopq %%rax\n"     \
            "\tjmp  *%c0(%%r11)\n" \
            : : "i" ((-lvo)*LIB_VECTSIZE) \
        ); \
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
 * the base in %r11, since the caller will
 * have used the AROS_LIBFUNCSTUB() macro.
 */

#define __GM_STRINGIZE(x) #x
#define __AROS_GM_STACKCALL(fname, libbasename, libfuncname) \
    void libfuncname(void); \
    void __ ## fname ## _stackcall(void) \
    { \
        asm volatile( \
            "\t" __GM_STRINGIZE(libfuncname) " :\n" \
            "\tpushq %%rax\n"     \
            "\tpushq %%rdi\n"     \
            "\tpushq %%rsi\n"     \
            "\tpushq %%rdx\n"     \
            "\tpushq %%rcx\n"     \
            "\tpushq %%r8\n"      \
            "\tpushq %%r9\n"      \
            "\tmovq  %%r11,%%rdi\n" \
            "\tcall __aros_setbase\n" \
            "\tpopq %%r9\n"      \
            "\tpopq %%r8\n"      \
            "\tpopq %%rcx\n"     \
            "\tpopq %%rdx\n"     \
            "\tpopq %%rsi\n"     \
            "\tpopq %%rdi\n"     \
            "\tpopq %%rax\n"     \
            "\tjmp  " #fname "\n" \
            : : : \
        ); \
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


#endif /* AROS_X86_64_GENMODULE_H */
