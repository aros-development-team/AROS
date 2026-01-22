/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include LC_LIBDEFS_FILE

/*****************************************************************************

    NAME */
//#include <libraries/vmm.h>

        AROS_LH2I(void, FreeVMem,

/*  SYNOPSIS */

        AROS_LHA(APTR, block,        A1),
        AROS_LHA(ULONG, size, D0),

/*  LOCATION */

        struct Library *, vmmBase, 6, VMM)

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

    AROS_LIBFUNC_EXIT
} /* FreeVMem */
