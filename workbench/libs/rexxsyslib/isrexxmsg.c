/*
        Copyright (C) 1995-2002, The AROS Development Team. All rights reserved.

    Desc:
*/
#include "rexxsyslib_intern.h"

/*****************************************************************************

    NAME */
#include <clib/rexxsyslib_protos.h>

        AROS_LH1(BOOL, IsRexxMsg,

/*  SYNOPSIS */
        AROS_LHA(struct RexxMsg *, msgptr, A0),

/*  LOCATION */
        struct Library *, RexxSysBase, 28, RexxSys)

/*  FUNCTION
        Tests whether a message node carries the identity marker used by
        this RexxSys library.

    INPUTS
        msgptr - Non-NULL pointer to a readable message node.

    RESULT
        TRUE if the node's ln_Name pointer matches the RexxSys marker;
        FALSE otherwise.

    NOTES
        This is a pointer-identity test, not validation of an arbitrary
        memory address or the contents of a complete RexxMsg structure.

    SEE ALSO
        CreateRexxMsg()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    BOOL ok = FALSE;
    char *name = ((struct Node *)msgptr)->ln_Name, *id = RSBI(RexxSysBase)->rexxmsgid;

    if (name == id) ok = TRUE;
  
    ReturnBool("IsRexxMsg", ok);
    AROS_LIBFUNC_EXIT
} /* IsRexxMsg */
