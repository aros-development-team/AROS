/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/types.h>

#include <proto/exec.h>

#include <limits.h>
#include <string.h>

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

	Function is not re-entrant. Results will be overwritten by
	subsequent calls.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct PosixCBase *PosixCBase = __aros_getbase_PosixCBase();
    struct PosixCIntBase *PosixCIntBase = (struct PosixCIntBase *)PosixCBase;
    char *s;

    /* quick and ugly... */
    if ((prompt) &&
        ((fputs(prompt, PosixCBase->_stdout) != EOF) &&
          (fputs("\n", PosixCBase->_stdout) != EOF)))
    {
        fflush(PosixCBase->_stdout);
    }
    s = fgets(PosixCIntBase->passbuffer, PASS_MAX, PosixCBase->_stdin);
    if (s)
    {
	/* strip trailing \n */
	size_t sl = strlen(s);
	if ( (sl > 0) && (s[sl-1] == '\n') )
	{
	    s[sl-1] = '\0';
	}
    }
    return s;
}
