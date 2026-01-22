/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include LC_LIBDEFS_FILE

/*****************************************************************************

    NAME */
//#include <libraries/vmm.h>

        AROS_LH2(APTR, AllocVMem,

/*  SYNOPSIS */

        AROS_LHA(ULONG, size,        D0),
        AROS_LHA(ULONG, attributes,       D1),

/*  LOCATION */

        struct Library *, vmmBase, 5, VMM)

/*  FUNCTION


    INPUTS


    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    return NULL;

    AROS_LIBFUNC_EXIT
} /* AllocVMem */
