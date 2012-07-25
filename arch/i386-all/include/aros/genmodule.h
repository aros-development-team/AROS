#ifndef AROS_I386_GENMODULE_H
#define AROS_I386_GENMODULE_H

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    NOTE: This file must compile *without* any other header !

    Desc: CPU-specific genmodule definitions for 32-bit x86 processors
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
            #fname " :\n" \
            "\tmovl " #libbasename ", %%eax\n" \
            "\tjmp *%c0(%%eax)\n" \
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
            "\t" #fname " :\n" \
            "\tcall __GM_GetBase\n" \
            "\taddl " #libbasename "_offset, %%eax\n" \
            "\tmovl (%%eax), %%eax\n" \
            "\tjmp *%c0(%%eax)\n" \
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

#endif /* AROS_I386_GENMODULE_H */
