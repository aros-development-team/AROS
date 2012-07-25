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

/* Macro for generating the __GM_GetBase() function
 * for a library. Optional for most architectures,
 * but we can save quite a bit of pushing and popping
 * with this optimized version.
 *
 * Pseudocode:
 * 
 * return SysBase->ThisTask->tc_UnionETask.tc_ETask.et_TaskStorage[__GM_BaseSlot];
 *
 * We can only use %rax and %r11
 * Base is in %rax on exit
 */
#define __AROS_GM_GETBASE() \
    asm volatile (  "    movq   SysBase(%%rip), %%rax\n"  \
                    "    movq   %c[task](%%rax), %%rax\n"  \
                    "    movq   %c[etask](%%rax), %%rax\n"  \
                    "    movq   %c[ts](%%rax), %%rax\n"  \
                    "    movslq __GM_BaseSlot(%%rip),%%r11\n"  \
                    "    movq   (%%rax,%%r11,8), %%rax\n" \
                 : : [task] "i"(offsetof(struct ExecBase, ThisTask)), \
                     [etask] "i"(offsetof(struct Task, tc_UnionETask.tc_ETask)), \
                     [ts] "i"(offsetof(struct ETask, et_TaskStorage)));

#define AROS_GM_GETBASE() \
    void __GM_GetBase_wrapper(void) { \
        asm volatile (  ".global __GM_GetBase\n" \
                        ".func   __GM_GetBase\n" \
                        "__GM_GetBase:\n"        \
        ); \
        __AROS_GM_GETBASE(); \
        asm volatile (  "retq\n" ); \
    }

/* Macro: AROS_LIBFUNCSTUB(functionname, libbasename, lvo)
   This macro will generate code for a stub function for
   the function 'functionname' of lirary with libbase
   'libbasename' and 'lvo' number of the function in the
   vector table. lvo has to be a constant value (not a variable)

   Internals: a dummy function is used that will generate some
   unused junk code but otherwise we can't pass input arguments
   to the asm statement
*/
#define __AROS_LIBFUNCSTUB(fname, libbasename, lvo) \
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
#define AROS_LIBFUNCSTUB(fname, libbasename, lvo) \
    __AROS_LIBFUNCSTUB(fname, libbasename, lvo)

/* Macro: AROS_RELLIBFUNCSTUB(functionname, libbasename, lvo)
   Same as AROS_LIBFUNCSTUB but finds libbase at an offset in
   the current libbase
*/
#define __AROS_RELLIBFUNCSTUB(fname, libbasename, lvo) \
    void __ ## fname ## _ ## libbasename ## _relwrapper(void) \
    { \
        asm volatile( \
            ".weak " #fname "\n" \
            "\t" #fname " :\n"); \
            __AROS_GM_GETBASE() \
        asm volatile( \
            "\taddq " #libbasename "_offset(%%rip), %%rax\n" \
            "\tmovq (%%rax),%%r11\n" \
            "\tjmp  *%c0(%%r11)\n" \
            : : "i" ((-lvo)*LIB_VECTSIZE) \
        ); \
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

#endif /* AROS_X86_64_GENMODULE_H */
