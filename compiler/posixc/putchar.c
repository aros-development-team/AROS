/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function putchar()
*/

#include <libraries/posixc.h>

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */

#include <stdio.h>

	int putchar(

/*  SYNOPSIS */
	int c)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct PosixCBase *PosixCBase = __aros_getbase_PosixCBase();

    D(bug("[putchar]PosixCBase: %p, stdout: %p\n", PosixCBase, PosixCBase->_stdout));

    return fputc(c, PosixCBase->_stdout);
}

