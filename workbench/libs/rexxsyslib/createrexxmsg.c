/*
        Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    Desc:
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
        struct RxsLib *, RexxSysBase, 24, RexxSys)

/*  FUNCTION
        Allocates and initializes a RexxMsg for communication with an
        ARexx interpreter or command host.

    INPUTS
        port      - Reply port to store in the new message. If the message
                    is sent, the port must remain valid until its reply.
        extension - Filename extension for macro lookup, or NULL if no
                    extension is supplied.
        host      - Initial command host name, or NULL if none is supplied.

    RESULT
        Pointer to the new message, or NULL if allocation fails.

    NOTES
        The message fields are initially cleared. The function stores the
        extension and host pointers without copying their strings. Any
        referenced storage must remain valid while the message uses it.
        Passing NULL for host leaves rm_CommAddr set to NULL; this
        function does not substitute the name "REXX".
        Delete the message with DeleteRexxMsg() after it is no longer in
        use, including by any pending recipient.

    SEE ALSO
        DeleteRexxMsg(), FillRexxMsg(), IsRexxMsg()

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
    
    ReturnPtrReg("CreateRexxMsg", struct RexxMsg *, rm, "a0");
    AROS_LIBFUNC_EXIT
} /* CreateRexxMsg */
