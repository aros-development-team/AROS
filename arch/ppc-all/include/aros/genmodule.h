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

/* Macro: AROS_LIBFUNCSTUB(functionname, libbasename, lvo)
   This macro will generate code for a stub function for
   the function 'functionname' of lirary with libbase
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
	    "\t" #fname ":\n" \
	    "\tlis   %%r12," #libbasename "@ha\n" \
	    "\tlwz   %%r12," #libbasename "@l(%%r12)\n" \
	    "\tlwz   %%r11,%c0(%%r12)\n" \
	    "\tmtctr %%r11\n" \
	    "\tbctr\n" \
	    : : "i" ((-lvo*LIB_VECTSIZE)) \
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
	    "\t" #fname ":\n" \
	    "\tstwu %%r1,-48(%%r1)\n"  /* Previous stack frame */\
	    "\tmflr %%r12\n"                                     \
	    "\tstw  %%r12,52(%%r1)\n"  /* LR                   */\
	    "\tstw  %%r3,8(%%r1)\n"    /* Arg 1                */\
	    "\tstw  %%r4,12(%%r1)\n"   /* Arg 2                */\
	    "\tstw  %%r5,16(%%r1)\n"   /* Arg 3                */\
	    "\tstw  %%r6,20(%%r1)\n"   /* Arg 4                */\
	    "\tstw  %%r7,24(%%r1)\n"   /* Arg 5                */\
	    "\tstw  %%r8,28(%%r1)\n"   /* Arg 6                */\
	    "\tstw  %%r9,32(%%r1)\n"   /* Arg 7                */\
	    "\tstw  %%r10,36(%%r1)\n"  /* Arg 8                */\
	    "\tbl   __aros_getbase\n"  /* base is in r3        */\
	    "\tlis  %%r12,__aros_rellib_offset_" #libbasename "@ha\n" \
	    "\tlwz  %%r12,__aros_rellib_offset_" #libbasename "@l(%%r12)\n" \
            "\tlwzx %%r12, %%r12, %%r3\n" /* Offset of library in base */\
	    "\taddis %%r11,%%r12,%c0@ha\n"                       \
	    "\tlwz  %%r11,%c0@l(%%r11)\n" /* Get LVO offset     */\
	    "\tlwz  %%r3,52(%%r1)\n"   /* LR (r3)              */\
	    "\tmtlr %%r3\n"            /* Restore LR from r3   */\
	    "\tlwz  %%r3,8(%%r1)\n"    /* Arg 1                */\
	    "\tlwz  %%r4,12(%%r1)\n"   /* Arg 2                */\
	    "\tlwz  %%r5,16(%%r1)\n"   /* Arg 3                */\
	    "\tlwz  %%r6,20(%%r1)\n"   /* Arg 4                */\
	    "\tlwz  %%r7,24(%%r1)\n"   /* Arg 5                */\
	    "\tlwz  %%r8,28(%%r1)\n"   /* Arg 6                */\
	    "\tlwz  %%r9,32(%%r1)\n"   /* Arg 7                */\
	    "\tlwz  %%r10,36(%%r1)\n"  /* Arg 8                */\
	    "\taddi %%r1,%%r1,48\n"    /* Destroy stack frame  */\
	    "\tmtctr %%r11\n"          /* r12 = base, r11 = lvo */\
	    "\tbctr\n" \
	    : : "i" (-(lvo*LIB_VECTSIZE)) \
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
 * the base in %r12, since the caller
 * will have used the AROS_GM_LIBFUNCSTUB() macro.
 */
#define __GM_STRINGIZE(x) #x
#define __AROS_GM_STACKCALL(fname, libbasename, libfuncname) \
    void libfuncname(void); \
    void __ ## fname ## _stackcall(void) \
    { \
        asm volatile( \
            "\t" __GM_STRINGIZE(libfuncname) " :\n" \
       	    "\tstwu %%r1,-48(%%r1)\n"  /* Previous stack frame */\
	    "\tmflr %%r11\n"                                      \
	    "\tstw  %%r11,52(%%r1)\n"  /* LR                   */\
	    "\tstw  %%r3,8(%%r1)\n"    /* Arg 1                */\
	    "\tstw  %%r4,12(%%r1)\n"   /* Arg 2                */\
	    "\tstw  %%r5,16(%%r1)\n"   /* Arg 3                */\
	    "\tstw  %%r6,20(%%r1)\n"   /* Arg 4                */\
	    "\tstw  %%r7,24(%%r1)\n"   /* Arg 5                */\
	    "\tstw  %%r8,28(%%r1)\n"   /* Arg 6                */\
	    "\tstw  %%r9,32(%%r1)\n"   /* Arg 7                */\
	    "\tstw  %%r10,36(%%r1)\n"  /* Arg 8                */\
	    "\tstw  %%r12,44(%%r1)\n"  /* current r12          */\
	    "\tmr   %%r3,%%r12\n"                                \
	    "\tbl   __aros_setbase\n"  /* base is in r3        */\
	    "\tlwz  %%r3,52(%%r1)\n"   /* LR (r3)              */\
	    "\tmtlr %%r3\n"            /* Restore LR from r3   */\
	    "\tlwz  %%r3,8(%%r1)\n"    /* Arg 1                */\
	    "\tlwz  %%r4,12(%%r1)\n"   /* Arg 2                */\
	    "\tlwz  %%r5,16(%%r1)\n"   /* Arg 3                */\
	    "\tlwz  %%r6,20(%%r1)\n"   /* Arg 4                */\
	    "\tlwz  %%r7,24(%%r1)\n"   /* Arg 5                */\
	    "\tlwz  %%r8,28(%%r1)\n"   /* Arg 6                */\
	    "\tlwz  %%r9,32(%%r1)\n"   /* Arg 7                */\
	    "\tlwz  %%r10,36(%%r1)\n"  /* Arg 8                */\
	    "\tlwz  %%r12,40(%%r1)\n"  /* Restore r12          */\
	    "\taddi %%r1,%%r1,48\n"    /* Destroy stack frame  */\
            "\tb " #fname "\n"  \
            :: \
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


#endif /* AROS_PPC_GENMODULE_H */
