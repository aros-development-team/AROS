/*
    Copyright (C) 1995-2008, The AROS Development Team. All rights reserved.

    Desc: Set a filecomment.
*/

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(LONG, SetComment,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name,    D1),
        AROS_LHA(CONST_STRPTR, comment, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 30, Dos)

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

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return SetCommentRelative(BNULL, name, comment);

    AROS_LIBFUNC_EXIT
} /* SetComment */
