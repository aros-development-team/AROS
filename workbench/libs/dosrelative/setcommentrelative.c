/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Set the comment of a file or directory relative to a directory lock.
*/

#include <proto/exec.h>
#include "dosrelative_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dosrelative.h>

        AROS_LH3(LONG, SetCommentRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         lock,    D1),
        AROS_LHA(CONST_STRPTR, name,    D2),
        AROS_LHA(CONST_STRPTR, comment, D3),

/*  LOCATION */
        struct DosRelativeBase *, DOSRelativeBase, 11, DosRelative)

/*  FUNCTION
        Change the comment on a file or directory, resolving a relative
        name against the given directory lock instead of the current
        directory of the calling process. The comment may be any
        NUL-terminated string. The supported size varies from filesystem
        to filesystem. In order to clear an existing comment, an empty
        string should be specified.

        If name is absolute (names a volume, device or assign, or begins
        with ':'), lock is ignored. If lock is NULL, the name is resolved
        against the current directory. In both cases the call behaves
        exactly like dos.library/SetComment().

    INPUTS
        lock    - directory lock to resolve a relative name against,
                  or NULL for the current directory
        name    - name of the file
        comment - new comment for the file.

    RESULT
        Boolean success indicator. IoErr() gives additional information
        upon failure.

    NOTES
        This call is AROS-specific.

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/SetComment()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BSTR bcomment;
    LONG status;

    D(bug("[SetCommentRelative] lock=0x%p '%s' '%s'\n", BADDR(lock), name, comment));

    if (name == NULL || comment == NULL)
        return DOSFALSE;

    if (lock == BNULL || dosrel_IsAbsolute(name))
        return SetComment(name, comment);

    if (strlen(comment) > MAXCOMMENTLENGTH)
    {
        SetIoErr(ERROR_COMMENT_TOO_BIG);
        return DOSFALSE;
    }

    bcomment = dosrel_C2BSTR(DOSRelativeBase, comment);
    if (bcomment == BNULL)
    {
        SetIoErr(ERROR_NO_FREE_STORE);
        return DOSFALSE;
    }

    status = dosrel_SendNamePkt(DOSRelativeBase, lock, name, ACTION_SET_COMMENT, (SIPTR)bcomment, 0, FALSE);
    dosrel_FreeBSTR(DOSRelativeBase, bcomment);

    return status;

    AROS_LIBFUNC_EXIT
} /* SetCommentRelative */
