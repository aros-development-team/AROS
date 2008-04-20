/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
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
	name	- name of the file
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

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys iofs;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_SET_COMMENT, DOSBase);

    iofs.io_Union.io_SET_COMMENT.io_Comment = comment;

    return DoIOFS(&iofs, NULL, name, DOSBase) == 0;

    AROS_LIBFUNC_EXIT
} /* SetComment */
