/*
        Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS


*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    BOOL ok = FALSE;
    char *name = ((struct Node *)msgptr)->ln_Name, *id = RSBI(RexxSysBase)->rexxmsgid;

    if (name == id) ok = TRUE;
  
    ReturnBool("IsRexxMsg", ok);
    AROS_LIBFUNC_EXIT
} /* IsRexxMsg */
