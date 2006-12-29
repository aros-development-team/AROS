/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Set a filecomment.
    Lang: english
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BOOL, SetComment,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name,    D1),
	AROS_LHA(STRPTR, comment, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 30, Dos)

/*  FUNCTION
	Change the comment on a file or directory.
	The comment may be any NUL terminated string. The supported
	size varies from filesystem to filesystem.

    INPUTS
	name	- name of the file
	comment - new comment for the file or NULL.

    RESULT
	!=0 if all went well, 0 else. IoErr() gives additional
	information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys iofs;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_SET_COMMENT, DOSBase);

    if(comment == NULL)
        comment = "";
    iofs.io_Union.io_SET_COMMENT.io_Comment = comment;

    return !DoName(&iofs, name, DOSBase);

    AROS_LIBFUNC_EXIT
} /* SetComment */
