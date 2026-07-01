/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: GetEntropyInfo() function.
*/

#include <exec/types.h>
#include <proto/exec.h>

#include "entropy_intern.h"

/*****************************************************************************

    NAME */
#include <proto/entropy.h>

        AROS_LH0(ULONG, GetEntropyInfo,

/*  SYNOPSIS */
        /* void */

/*  LOCATION */
        struct EntropyBase *, EntropyBase, 3, Entropy)

/*  FUNCTION
        Report which entropy sources the running resource is drawing on.

    INPUTS
        None.

    RESULT
        A mask of EIF_* flags (see <resources/entropy.h>):

        EIF_SOFTWARE - the generic software collector is active (always set).
        EIF_HARDWARE - a dedicated CPU/board hardware entropy source is in use.

        The identity of any hardware source (for example which x86 instruction
        is used) is an architecture-specific implementation detail and is
        deliberately not reported here.

    NOTES
        The flags are established once at resource initialisation and do not
        change, so no locking is required to read them.

    EXAMPLE
        if (GetEntropyInfo() & EIF_HARDWARE)
            ; // a hardware entropy source is in use

    BUGS

    SEE ALSO
        GetEntropy(), AddEntropy()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return EntropyBase->eb_Flags;

    AROS_LIBFUNC_EXIT
} /* GetEntropyInfo */
