/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS specific function for environ emulation handling
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>

#include "__arosc_privdata.h"
#include "__env.h"

#include <stdio.h>

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

	void __arosc_set_environptr (

/*  SYNOPSIS */
	char ***environptr)

/*  FUNCTION
	This function is called to enable environ emulation mode.

    INPUTS
	environptr - ptr to the child environ variable (== &environ).

    RESULT
        -

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
        getenv(), setenv()

    INTERNALS

******************************************************************************/
{
    struct aroscbase *aroscbase = __GM_GetBase();
    int len;

    D(bug("Initializing POSIX environ emulation\n"));

    aroscbase->acb_environptr = environptr;

    len = __env_get_environ(NULL, 0);
    *environptr = malloc(len);
    if (__env_get_environ(*environptr, len) < 0)
    {
        fputs("Internal error with environ initialization\n", stderr);
        abort();
    }
}
