/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Set a filecomment.
    Lang: english
*/
#define DEBUG 0
#include <aros/debug.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/dos.h>
#include <proto/dos.h>
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

    struct PacketHelperStruct phs;
    LONG status = DOSFALSE;

    D(bug("[SetComment] '%s' '%s'\n", name, comment));
    if (strlen(comment)>MAXCOMMENTLENGTH)
    {
        SetIoErr(ERROR_COMMENT_TOO_BIG);
        return status;
    }
    if (getpacketinfo(DOSBase, name, &phs)) {
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
} /* SetComment */
