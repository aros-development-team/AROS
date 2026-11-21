/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    m68kjit.library: the one function of the library.
*/
#include <aros/libcall.h>
#include <exec/types.h>
#include <proto/exec.h>

#include "m68kjit_intern.h"

/* LVO -30, the first function after the standard vectors: the engine's
   table of operations for an interface major, or NULL. */
AROS_LH1(APTR, M68KJit_GetEngine,
         AROS_LHA(ULONG, abiMajor, D0),
         struct Library *, M68KJitBase, 5, M68KJit)
{
    AROS_LIBFUNC_INIT

    return (APTR)M68KJit_GetEngineTable(abiMajor);

    AROS_LIBFUNC_EXIT
}
