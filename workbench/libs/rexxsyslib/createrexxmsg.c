/*
        Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "rexxsyslib_intern.h"
#include <stdio.h>
#include <string.h>

/*****************************************************************************

    NAME */
#include <clib/rexxsyslib_protos.h>

	AROS_LH3(struct RexxMsg *, CreateRexxMsg,

/*  SYNOPSIS */
	AROS_LHA(struct MsgPort *, port     , A0),
	AROS_LHA(UBYTE          *, extension, A1),
	AROS_LHA(UBYTE          *, host     , D0),

/*  LOCATION */
	struct Library *, RexxSysBase, 24, RexxSys)

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
    struct RexxMsg *rm = AllocMem(sizeof(struct RexxMsg), MEMF_PUBLIC|MEMF_CLEAR);

    if (rm == NULL) ReturnPtr("CreateRexxMsg", struct RexxMsg *, NULL);

    rm->rm_Node.mn_Node.ln_Type = NT_MESSAGE;
    rm->rm_Node.mn_Node.ln_Name = RSBI(RexxSysBase)->rexxmsgid;
    rm->rm_Node.mn_ReplyPort = port;
    rm->rm_Node.mn_Length = sizeof(struct RexxMsg);
    rm->rm_FileExt = (STRPTR)extension;
    rm->rm_CommAddr = (STRPTR)host;
    
    ReturnPtr("CreateRexxMsg", struct RexxMsg *, rm);
    AROS_LIBFUNC_EXIT
} /* CreateRexxMsg */
