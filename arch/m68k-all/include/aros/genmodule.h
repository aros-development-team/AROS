#ifndef AROS_M68K_GENMODULE_H
#define AROS_M68K_GENMODULE_H

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc:CPU-specific genmodule definitions for Motorola m68k processors
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
#define __AROS_LIBFUNCSTUB(fname, libbasename, lvo) \
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
#define AROS_LIBFUNCSTUB(fname, libbasename, lvo) \
    __AROS_LIBFUNCSTUB(fname, libbasename, lvo)

/* Macro: AROS_RELLIBFUNCSTUB(functionname, libbasename, lvo)
   Same as AROS_LIBFUNCSTUB but finds libbase at an offset in
   the current libbase

   For the m68k C ABI, all operands are on the stack, so
   'scratch' registers d0-d1/a0-a1 are free for use by this
   stub.
*/
#define __AROS_RELLIBFUNCSTUB(fname, libbasename, lvo) \
    void __ ## fname ## _ ## libbasename ## _relwrapper(IPTR args) \
    { \
        asm volatile( \
            ".weak " #fname "\n" \
            #fname " :\n" \
            "\tjsr __GM_GetBase\n" \
            "\tmove.l " #libbasename "_offset, %%a0\n" \
            "\tmove.l %%a0@(%%d0), %%a1\n" \
            "\tjmp %%a1@(%c0)\n" \
            : : "i" ((-lvo*LIB_VECTSIZE)) \
            : \
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

#endif /* AROS_M68K_GENMODULE_H */
