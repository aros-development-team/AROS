/*
    Copyright (C) 1995-2008, The AROS Development Team. All rights reserved.

    Desc: Set a filecomment.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/dos.h>
#include <proto/dos.h>

#include <string.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH3(LONG, SetCommentRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         lock, D1),
        AROS_LHA(CONST_STRPTR, name,    D2),
        AROS_LHA(CONST_STRPTR, comment, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 233, Dos)

/*  FUNCTION
        Change the comment on a file or directory. The comment may be any
        NUL-terminated string. The supported size varies from filesystem
        to filesystem. In order to clear an existing comment, an empty
        string should be specified.

    INPUTS
        name    - name of the file
        comment - new comment for the file.

    RESULT
        Boolean success indicator. IoErr() gives additional information upon
        failure.

    NOTES
        This call is AROS-specific.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PacketHelperStruct phs;
    LONG status = DOSFALSE;

    D(bug("[SetCommentRelative] lock=0x%p '%s' '%s'\n", lock, name, comment));
    if (strlen(comment)>MAXCOMMENTLENGTH)
    {
        SetIoErr(ERROR_COMMENT_TOO_BIG);
        return status;
    }
    if (getpacketinfo(DOSBase, lock, name, &phs)) {
        BSTR com = C2BSTR(comment);
        if (com) {
            status = dopacket4(DOSBase, NULL, phs.port, ACTION_SET_COMMENT, (SIPTR)NULL, phs.lock, phs.name, com);
            FREEC2BSTR(com);
        } else {
            SetIoErr(ERROR_NO_FREE_STORE);
        }
        freepacketinfo(DOSBase, &phs);
    }

    return status;

    AROS_LIBFUNC_EXIT
} /* SetCommentRelative */
