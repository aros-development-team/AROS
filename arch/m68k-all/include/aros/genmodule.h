#ifndef AROS_M68K_GENMODULE_H
#define AROS_M68K_GENMODULE_H

/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc:CPU-specific genmodule definitions for Motorola m68k processors
    Lang: english
*/

#include <exec/execbase.h>

/* Macros for generating library stub functions and aliases for stack libcalls. */

/******************* Linklib Side Thunks ******************/

/* The library base travels in %a1, not %a0.
   GCC m68k passes the hidden result pointer of a function that returns a
   struct (div_t, double complex ...) in %a0; the old stubs loaded the base
   into %a0 and so lost it: cexp() wrote its 16-byte result over the start
   of the stdc base (GCC turns sin(x) and cos(x) of one x into cexp()).
   Stubs and the library-side thunk below keep %a0 intact. */

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
            #fname ":\n" \
            "\tmove.l   " #libbasename ",%%a1\n" \
            "\tjmp %%a1@(%c0)\n" \
            : : "i" ((-lvo*LIB_VECTSIZE)) \
        ); \
    }
#define AROS_GM_LIBFUNCSTUB(fname, libbasename, lvo) \
    __AROS_GM_LIBFUNCSTUB(fname, libbasename, lvo)

/* Macro: AROS_GM_RELLIBFUNCSTUB(functionname, libbasename, lvo)
   Same as AROS_GM_LIBFUNCSTUB but finds libbase at an offset in
   the current libbase

   For the m68k C ABI, all operands are on the stack, so
   'scratch' registers d0-d1/a1 are free for use by this
   stub; %a0 may hold a struct return pointer and is kept
   (__aros_getoffsettable is a C function: it may clobber it).
*/
#define __AROS_GM_RELLIBFUNCSTUB(fname, libbasename, lvo) \
    void __ ## fname ## _ ## libbasename ## _relwrapper(IPTR args) \
    { \
        asm volatile( \
            ".weak " #fname "\n" \
            #fname " :\n" \
            "\tmove.l %%a0,%%sp@-\n" \
            "\tjsr __aros_getoffsettable\n" \
            "\tmove.l __aros_rellib_offset_" #libbasename ", %%a1\n" \
            "\tmove.l %%a1@(%%d0), %%a1\n" \
            "\tmove.l %%sp@+,%%a0\n" \
            "\tjmp %%a1@(%c0)\n" \
            : : "i" ((-lvo*LIB_VECTSIZE)) \
            : \
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
 * the base in %a1, instead of %a6, since the caller
 * will have used the AROS_LIBFUNCSTUB() macro.
 * %a0 (struct return pointer) is kept across the C call.
 */
#define __GM_STRINGIZE(x) #x
#define __AROS_GM_STACKCALL(fname, libbasename, libfuncname) \
    void libfuncname(void); \
    void __ ## fname ## _stackcall(void) \
    { \
        asm volatile( \
            "\t" __GM_STRINGIZE(libfuncname) " :\n" \
            "\tmove.l %a0,%sp@-\n" \
            "\tmove.l %a1,%sp@-\n" \
            "\tjsr __aros_setoffsettable\n" \
            "\taddq.l #4,%sp\n"     \
            "\tmove.l %sp@+,%a0\n" \
            "\tjmp  " #fname "\n"  \
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



#endif /* AROS_M68K_GENMODULE_H */
