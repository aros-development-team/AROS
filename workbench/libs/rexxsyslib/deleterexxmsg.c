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

	AROS_LH1(VOID, DeleteRexxMsg,

/*  SYNOPSIS */
	AROS_LHA(struct RexxMsg *, packet, A0),

/*  LOCATION */
	struct RxsLib *, RexxSysBase, 25, RexxSys)

/*  FUNCTION
         Deletes a RexxMsg structure

    INPUTS
         packet - The RexxMsg to delete.

    RESULT
         void

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
         CreateRexxMsg()

    INTERNALS


*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    FreeMem(packet, packet->rm_Node.mn_Length);
  
    ReturnVoid("DeleteRexxMsg");
    AROS_LIBFUNC_EXIT
} /* DeleteRexxMsg */
