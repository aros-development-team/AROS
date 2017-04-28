/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/types.h>

#include <exec/tasks.h>
#include <proto/exec.h>

#include <assert.h>

#include "__vfork.h"
#include "__posixc_intbase.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	char * getpass(

/*  SYNOPSIS */
	const char *prompt)

/*  FUNCTION
	(obsolete) prompt for a password.

    INPUTS

    RESULT

    NOTES
	This function returns a pointer to a static buffer
	containing (the first PASS_MAX bytes of) the password without the
	trailing newline, terminated by a null byte ('\0').

	The buffer may be overwritten by a subsequent call

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCIntBase = (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    return PosixCIntBase->passbuffer;
}
