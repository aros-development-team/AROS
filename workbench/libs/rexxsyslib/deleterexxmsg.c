/*
        Copyright (C) 1995-2002, The AROS Development Team. All rights reserved.

    Desc:
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
        Frees the storage of a RexxMsg created with CreateRexxMsg().
        Referenced argument strings and other resources are not freed.

    INPUTS
        packet - Non-NULL RexxMsg returned by CreateRexxMsg(). The
                 message must no longer be in use.

    RESULT
        None.

    NOTES
        Wait for any outstanding reply before deleting a sent message.
        Release argument strings owned by the caller with ClearRexxMsg()
        before deleting the message. The function does not free strings
        referenced by rm_CommAddr or rm_FileExt.

    SEE ALSO
        CreateRexxMsg(), ClearRexxMsg(), DeleteArgstring()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    FreeMem(packet, packet->rm_Node.mn_Length);
  
    ReturnVoid("DeleteRexxMsg");
    AROS_LIBFUNC_EXIT
} /* DeleteRexxMsg */
