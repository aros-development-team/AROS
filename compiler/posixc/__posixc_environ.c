/*
    Copyright © 2012-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS specific function for environ emulation handling
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>

#include "__posixc_intbase.h"
#include "__env.h"

#include <stdio.h>

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

	int __posixc_set_environptr (

/*  SYNOPSIS */
	char ***environptr)

/*  FUNCTION
	This function is called to enable environ emulation mode.

    INPUTS
	environptr - ptr to the child environ variable (== &environ).

    RESULT
        0 on fail, other value on succes

    NOTES
        This function will enable environ emulation. This means that
        all current DOS local variables are converted to the 'var=value'
        format and be accessible through char **environ.

    EXAMPLE

    BUGS
        At the moment only a static list is supported. getenv() and setenv()
        don't use this yet so changes done with these functions are not
        reflected in environ.
        This is still TODO.

    SEE ALSO
        __posixc_get_environptr(), getenv(), setenv()

    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    int len;

    D(bug("Initializing POSIX environ emulation\n"));

    PosixCBase->environptr = environptr;

    len = __env_get_environ(NULL, 0);
    *environptr = malloc(len);
    return (__env_get_environ(*environptr, len) >= 0);
}

/*****************************************************************************

    NAME */
#include <stdlib.h>

	char ***__posixc_get_environptr (

/*  SYNOPSIS */
	void)

/*  FUNCTION
	This function the get pointer to the child environ global variable
        currently used by posixc.library.

    INPUTS
        -

    RESULT 
        environptr - ptr to the child environ variable (== &environ).
                     NULL is return if envirion emulation is disabled.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        __posixc_set_environptr()

    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    return PosixCBase->environptr;
}
