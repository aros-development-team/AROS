#ifndef AROS_PPC_GENMODULE_H
#define AROS_PPC_GENMODULE_H
/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    NOTE: This file must compile *without* any other header !

    Desc: genmodule.h include file for powerpc arch
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
 * return SysBase->ThisTask->tc_UnionETask.tc_ETask->et_TaskStorage[__GM_BaseSlot];
 *
 * We can only use registers %r0, %r11, and %r12
 * On exit, %r11 will have the base or NULL
 */
#define __AROS_GM_GETBASE() \
    asm volatile ( \
            "    lis   %%r11,SysBase@ha\n" \
            "    lwz   %%r11,SysBase@l(%%r11)\n" \
            "    lwz   %%r11,%c[task](%%r11)\n" \
            "    lwz   %%r11,%c[etask](%%r11)\n" \
            "    lwz   %%r11,%c[ts](%%r11)\n" \
            "    lis   %%r12,__GM_BaseSlot@ha\n" \
            "    lwz   %%r12,__GM_BaseSlot@l(%%r12)\n" \
            "    slwi  %%r12,%%r12,2\n" \
            "    lwzx  %%r11,%%r12,%%r11\n" \
            "1:\n" \
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
        asm volatile (  "    mr %r3,%r11\n" \
                        "    blr\n" ); \
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
	    "\t" #fname ":\n" \
	    "\tlis   12," #libbasename "@ha\n" \
	    "\tlwz   12," #libbasename "@l(12)\n" \
	    "\tlwz   11,%c0(12)\n" \
	    "\tmtctr 11\n" \
	    "\tbctr\n" \
	    : : "i" ((-lvo*LIB_VECTSIZE)) \
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
	    "\t" #fname ":\n" \
        ); \
	    __AROS_GM_GETBASE() \
	asm volatile( \
	    "\tlis  %%r12," #libbasename "_offset@ha\n" \
	    "\tlwz  %%r12," #libbasename "_offset@l(%%r12)\n" \
            "\tadd  %%r11, %%r12, %%r11\n" \
	    "\tlis  %%r12, %c0@ha\n" \
	    "\tlwz  %%r12, %c0@l(%%r12)\n" \
	    "\tadd  %%r11, %%r11, %%r12\n" \
	    "\tmtctr %%r11\n" \
	    "\tbctr\n" \
	    : : "i" ((-lvo*LIB_VECTSIZE)) \
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

#endif /* AROS_PPC_GENMODULE_H */
